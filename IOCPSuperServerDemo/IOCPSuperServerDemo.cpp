// IOCPServerDemo.cpp : 定义控制台应用程序的入口点。
//


/*
完成端口会充分利用Windows内核来进行I/O的调度，是用于C/S通信模式中性能最好的网络通信模型；

注意：
使用AcceptEx()，使用完成端口的方式接受客户端的连接，让服务程序并发性能更加强大；
*/

#include "stdafx.h"

#include <iostream>

#include <WINSOCK2.H>
#include <ws2tcpip.h>

#include <mswsock.h>     //微软扩展的类库AcceptEx()

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "Mswsock.lib")

#pragma warning(disable:4996)

using namespace std;

#define PORT    6700
#define MSGSIZE 1024

//完成键
typedef struct
{
	SOCKET ckSocket;
	SOCKADDR_IN ckAddr;
} COMPLETIONKEY, *PCOMPLETIONKEY;

//操作类型
typedef enum
{
	ACCEPTEX_POSTED,
	RECV_POSTED,
	SEND_POSTED
} OPERATION_TYPE;

//IO操作数据
typedef struct
{
	WSAOVERLAPPED Overlapped;
	WSABUF         Buff;	//数据缓冲区,用于WSASend/WSARecv
	DWORD          NumberOfBytesRecvd;
	DWORD          NumberOfBytesSended;
	DWORD          Flags;

	char           BuffData[MSGSIZE];
	OPERATION_TYPE OperationType;

	SOCKET newClient;
} PER_IO_OPERATION_DATA, *LPPER_IO_OPERATION_DATA;

//这两个函数是扩展库的，通过函数指针调用，不用每次都去调用WSAIoctl
LPFN_ACCEPTEX lpfAcceptEx = NULL;
LPFN_GETACCEPTEXSOCKADDRS lpfGetAcceptExSockAddrs = NULL;

//工作者线程
DWORD WINAPI WorkerThread(LPVOID);

int main()
{
	//初始化SOCKET库，设置版本
	WSADATA wsaData;
	WSAStartup(0x0202, &wsaData);

	//创建IO完成端口
	HANDLE CompletionPort = INVALID_HANDLE_VALUE;
	CompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

	//创建工作者线程，线程个数是CPU核心的数量
	SYSTEM_INFO systeminfo;
	GetSystemInfo(&systeminfo);
	for (DWORD i = 0; i < systeminfo.dwNumberOfProcessors * 2; i++)
	{
		CreateThread(NULL, 0, WorkerThread, CompletionPort, 0, NULL);
		cout << "Create Worker Thread " << i + 1 << endl;
	}

	//创建IO重叠的SOCKET，指定套接字的类型、使用的协议
	SOCKET sListen = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);

	//获得AcceptEx和GetAcceptExSockaddrs函数指针
	GUID guidAcceptEx = WSAID_ACCEPTEX;
	DWORD dwBytes = 0;
	if (WSAIoctl(sListen, SIO_GET_EXTENSION_FUNCTION_POINTER,
		&guidAcceptEx, sizeof(guidAcceptEx), &lpfAcceptEx, sizeof(lpfAcceptEx),
		&dwBytes, NULL, NULL) != 0)
	{
		cout << "Error: Get AcceptEx funcation fail." << endl;
	}

	GUID guidGetAcceptExSockAddrs = WSAID_GETACCEPTEXSOCKADDRS;
	dwBytes = 0;
	if (WSAIoctl(sListen, SIO_GET_EXTENSION_FUNCTION_POINTER,
		&guidGetAcceptExSockAddrs, sizeof(guidGetAcceptExSockAddrs), &lpfGetAcceptExSockAddrs, sizeof(lpfGetAcceptExSockAddrs),
		&dwBytes, NULL, NULL) != 0)
	{
		cout << "Error: Get GetAcceptExSockAddrs funcation fail." << endl;
	}

	//绑定端口
	SOCKADDR_IN localAddr;
	localAddr.sin_family = AF_INET;
	localAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	//localAddr.sin_addr.S_un.S_addr = inet_addr("192.168.249.134");
	localAddr.sin_port = htons(PORT);
	bind(sListen, (struct sockaddr *)&localAddr, sizeof(SOCKADDR_IN));
	cout << "bind()" << endl;

	//将IO完成端口和监听Socket绑定，处理AcceptEx重叠IO操作
	PCOMPLETIONKEY pckServer = new COMPLETIONKEY;
	pckServer->ckSocket = sListen;
	pckServer->ckAddr = localAddr;
	CreateIoCompletionPort((HANDLE)sListen, CompletionPort, (DWORD)pckServer, 0);

	//开始监听，监听队列的最大数量
	listen(sListen, SOMAXCONN);
	cout << "listen()" << endl;

	//预备N个异步连接
	for (int i = 0; i < systeminfo.dwNumberOfProcessors * 5; i++)
	{
		//异步连接客户端，并读取数据，后面加上本地地址和远程地址
		LPPER_IO_OPERATION_DATA lpPerIOData = NULL;
		lpPerIOData = (LPPER_IO_OPERATION_DATA)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(PER_IO_OPERATION_DATA));

		lpPerIOData->Buff.len = MSGSIZE;
		lpPerIOData->Buff.buf = lpPerIOData->BuffData;
		lpPerIOData->newClient = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);	//先创建Socket，节省时间
		lpPerIOData->OperationType = ACCEPTEX_POSTED;

		int rc = lpfAcceptEx(
			sListen, 
			lpPerIOData->newClient, 
			lpPerIOData->Buff.buf,
			lpPerIOData->Buff.len - ((sizeof(SOCKADDR_IN) + 16) * 2),
			sizeof(SOCKADDR_IN) + 16, 
			sizeof(SOCKADDR_IN) + 16, 
			&lpPerIOData->NumberOfBytesRecvd,
			&lpPerIOData->Overlapped);
		if (rc == FALSE)
		{
			if (WSAGetLastError() != ERROR_IO_PENDING) cout << "Error: Invoke lpfnAcceptEx failed." << endl;
		}
	}

	//等待输入退出程序
	int i = 0;
	cin >> i;

	//投递退出消息到队列
	for (DWORD i = 0; i < systeminfo.dwNumberOfProcessors * 2; i++)
	{
		PostQueuedCompletionStatus(CompletionPort, 0, 0, NULL);
	}

	//关闭句柄
	CloseHandle(CompletionPort);

	//关闭服务端Socket
	closesocket(sListen);
	WSACleanup();

	return 0;
}

DWORD WINAPI WorkerThread(LPVOID CompletionPortID)
{
	//IO完成端口
	HANDLE CompletionPort = (HANDLE)CompletionPortID;

	//完成键
	PCOMPLETIONKEY pckData = NULL;

	//IO操作数据
	LPPER_IO_OPERATION_DATA pioData = NULL;

	DWORD dwBytesTransferred;
	const char* szRespone = "OK";

	while (true)
	{
		//获取队列中处理完成的IO操作
		BOOL bSuccess = GetQueuedCompletionStatus(CompletionPort,
			&dwBytesTransferred,
			(LPDWORD)&pckData,
			(LPOVERLAPPED *)&pioData,
			INFINITE);

		//关闭句柄
		if (FALSE == bSuccess) break;

		//程序退出
		if (0 == pckData) break;

		//判断完成端口的消息类型
		if (ACCEPTEX_POSTED == pioData->OperationType)
		{
			//处理AcceptEx()操作的结果======================================
			//获取数据，本地地址和远程地址
			SOCKET mClient = pioData->newClient;
			int recvLen = dwBytesTransferred;
			char recvData[MSGSIZE];
			memcpy(recvData, pioData->BuffData, recvLen);
			recvData[recvLen] = '\0';

			SOCKADDR_IN localAddr, peerAddr;
			int localLen = sizeof(SOCKADDR_IN), peerLen = sizeof(SOCKADDR_IN);
			lpfGetAcceptExSockAddrs(pioData->Buff.buf,
				pioData->Buff.len - ((sizeof(SOCKADDR_IN) + 16) * 2),
				sizeof(SOCKADDR_IN) + 16,
				sizeof(SOCKADDR_IN) + 16,
				(LPSOCKADDR*)&localAddr,
				&localLen,
				(LPSOCKADDR*)&peerAddr,
				&peerLen);

			//继续发起AcceptEx接受其他客户端的连接
			ZeroMemory(pioData, sizeof(pioData));
			pioData->Buff.len = MSGSIZE;
			pioData->Buff.buf = pioData->BuffData;
			pioData->newClient = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);	//先创建Socket，节省时间
			pioData->OperationType = ACCEPTEX_POSTED;

			int rc = lpfAcceptEx(
				pckData->ckSocket,
				pioData->newClient,
				pioData->Buff.buf,
				pioData->Buff.len - ((sizeof(SOCKADDR_IN) + 16) * 2),
				sizeof(SOCKADDR_IN) + 16,
				sizeof(SOCKADDR_IN) + 16,
				&pioData->NumberOfBytesRecvd,
				&pioData->Overlapped);

			//处理请求，发送响应
			printf("New Client[%s#%d]: %s\n", inet_ntoa(peerAddr.sin_addr), ntohs(peerAddr.sin_port), recvData);

			send(mClient, szRespone, strlen(szRespone), 0);

			//将IO完成端口和新的客户端Socket绑定
			PCOMPLETIONKEY pckClient = new COMPLETIONKEY;
			pckClient->ckSocket = mClient;
			pckClient->ckAddr = peerAddr;
			CreateIoCompletionPort((HANDLE)pckClient->ckSocket, CompletionPort, (DWORD)pckClient, 0);

			//对新的客户端发起异步IO操作 - WSARecv()：发送数据(WSASend和WSASendTo)，接收数据(WSARecv和WSARecvFrom)
			LPPER_IO_OPERATION_DATA lpPerIOData = NULL;
			lpPerIOData = (LPPER_IO_OPERATION_DATA)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(PER_IO_OPERATION_DATA));

			lpPerIOData->Buff.len = MSGSIZE;
			lpPerIOData->Buff.buf = lpPerIOData->BuffData;
			lpPerIOData->OperationType = RECV_POSTED;

			WSARecv(pckClient->ckSocket,
				&lpPerIOData->Buff,
				1,
				&lpPerIOData->NumberOfBytesRecvd,
				&lpPerIOData->Flags,
				&lpPerIOData->Overlapped,
				NULL);
		}
		else if (RECV_POSTED == pioData->OperationType)
		{
			//处理WSARecv()操作的结果======================================
			if (dwBytesTransferred == 0)
			{
				//客户端关闭连接
				closesocket(pckData->ckSocket);
				HeapFree(GetProcessHeap(), 0, pioData);
				//cout << "closesocket()" << endl;
			}
			else
			{
				//发送请求响应
				pioData->BuffData[dwBytesTransferred] = '\0';	//发送一样的信息到客户端
				cout << "Rev() = " << pioData->BuffData << endl;

				//以阻塞方式发送响应
				send(pckData->ckSocket, szRespone, strlen(szRespone), 0);

				//继续发起异步IO操作 - WSARecv()
				memset(pioData, 0, sizeof(PER_IO_OPERATION_DATA));
				pioData->Buff.len = MSGSIZE;
				pioData->Buff.buf = pioData->BuffData;
				pioData->OperationType = RECV_POSTED;

				WSARecv(pckData->ckSocket,
					&pioData->Buff,
					1,
					&pioData->NumberOfBytesRecvd,
					&pioData->Flags,
					&pioData->Overlapped,
					NULL);
			}
		}
		else if (SEND_POSTED == pioData->OperationType)
		{



		}
		else
		{
			cout << "Error: unknown operation." << endl;
		}
	}

	return 0;
}

