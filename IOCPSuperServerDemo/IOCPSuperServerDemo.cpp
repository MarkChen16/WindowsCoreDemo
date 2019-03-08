// IOCPServerDemo.cpp : 定义控制台应用程序的入口点。
//


/*
完成端口会充分利用Windows内核来进行I/O的调度，是用于C/S通信模式中性能最好的网络通信模型；

注意：
使用AcceptEx()，使用完成端口的方式接受客户端的连接，让服务程序并发性能更加强大；

Windows 下单机最大TCP连接数
调整系统参数来调整单机的最大TCP连接数，Windows 下单机的TCP连接数有多个参数共同决定：
以下都是通过修改注册表[HKEY_LOCAL_MACHINE \System \CurrentControlSet \Services \Tcpip \Parameters]
1. 最大TCP连接数             TcpNumConnections
2. TCP关闭延迟时间           TCPTimedWaitDelay    (30-240)s        
3. 最大动态端口数            MaxUserPort  (Default = 5000, Max = 65534) TCP客户端和服务器连接时，客户端必须分配一个动态端口，默认情况下这个动态端口的分配范围为 1024-5000 ，也就是说默认情况下，客户端最多可以同时发起3977 个Socket 连接    
4. 最大TCB 数量              MaxFreeTcbs系统为每个TCP 连接分配一个TCP 控制块(TCP control block or TCB)，这个控制块用于缓存TCP连接的一些参数，每个TCB需要分配 0.5 KB的pagepool 和 0.5KB 的Non-pagepool，也就说，每个TCP连接会占用 1KB 的系统内存。非Server版本，MaxFreeTcbs 的默认值为1000 （64M 以上物理内存）Server 版本，这个的默认值为 2000。也就是说，默认情况下，Server 版本最多同时可以建立并保持2000个TCP 连接。
5. 最大TCB Hash table 数量   MaxHashTableSize TCB 是通过Hash table 来管理的。这个值指明分配 pagepool 内存的数量，也就是说，如果MaxFreeTcbs = 1000 , 则 pagepool 的内存数量为 500KB那么 MaxHashTableSize 应大于 500 才行。这个数量越大，则Hash table 的冗余度就越高，每次分配和查找 TCP  连接用时就越少。这个值必须是2的幂，且最大为65536.
*/

#include "stdafx.h"

#include <iostream>

#include <vector>

#include <WINSOCK2.H>
#include <ws2tcpip.h>
#include <mswsock.h>     //微软扩展的类库AcceptEx()

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "Mswsock.lib")

#pragma warning(disable:4996)

using namespace std;

//监听的端口
#define PORT    6700

//缓存使用4K，一个内存页面大小
#define MSGSIZE 4096 * 10

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

	//创建工作者线程，线程个数是CPU核心的两倍数量
	SYSTEM_INFO systeminfo;
	GetSystemInfo(&systeminfo);
	int cntWorker = systeminfo.dwNumberOfProcessors * 2 + 2;

	for (int i = 0; i < cntWorker; i++)
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
	localAddr.sin_addr.S_un.S_addr = inet_addr("192.168.249.134");
	localAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

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

	//投递N个异步连接操作，当系统连接客户端并读取第一块数据后，添加到完成端口队列中；
	cout << "Max Connection: ";
	int cntMaxConnection = 0;
	cin >> cntMaxConnection;

	vector<LPPER_IO_OPERATION_DATA> lstAcceptIOData;
	for (int i = 0; i < cntMaxConnection; i++)
	{
		//异步连接客户端，并读取数据，后面加上本地地址和远程地址
		LPPER_IO_OPERATION_DATA lpPerIOData = NULL;
		lpPerIOData = (LPPER_IO_OPERATION_DATA)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(PER_IO_OPERATION_DATA));
		lstAcceptIOData.push_back(lpPerIOData);

		lpPerIOData->Buff.len = MSGSIZE;
		lpPerIOData->Buff.buf = lpPerIOData->BuffData;
		lpPerIOData->OperationType = ACCEPTEX_POSTED;

		//先创建Socket，节省时间
		lpPerIOData->newClient = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);

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

	cout << "init all acceptex..." << endl;

	//等待输入退出程序
	int i = 0;
	std::cin >> i;

	//投递退出消息到队列
	for (int i = 0; i < cntWorker; i++)
	{
		PostQueuedCompletionStatus(CompletionPort, 0, 0, NULL);
	}

	//关闭句柄
	CloseHandle(CompletionPort);

	//关闭服务端Socket
	closesocket(sListen);
	WSACleanup();

	//清除AcceptEx的IO操作数据
	for each (auto var in lstAcceptIOData)
	{
		HeapFree(GetProcessHeap(), 1, var);
	}
	lstAcceptIOData.clear();

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

			LPSOCKADDR localAddr, peerAddr;
			int localLen = 0, peerLen = 0;

			lpfGetAcceptExSockAddrs(pioData->Buff.buf,
				pioData->Buff.len - ((sizeof(SOCKADDR_IN) + 16) * 2),
				sizeof(SOCKADDR_IN) + 16,
				sizeof(SOCKADDR_IN) + 16,
				(LPSOCKADDR*)&localAddr,
				&localLen,
				(LPSOCKADDR*)&peerAddr,
				&peerLen);

			SOCKADDR_IN clientAddr;
			memcpy(&clientAddr, peerAddr, sizeof(clientAddr));
			printf("New Client[%s:%d]: %s\n", inet_ntoa(clientAddr.sin_addr), clientAddr.sin_port, recvData);

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
			send(mClient, szRespone, strlen(szRespone), 0);

			//将IO完成端口和新的客户端Socket绑定
			PCOMPLETIONKEY pckClient = new COMPLETIONKEY;
			pckClient->ckSocket = mClient;
			pckClient->ckAddr = clientAddr;
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

