// IOCPServerDemo.cpp : 定义控制台应用程序的入口点。
//


/*
完成端口会充分利用Windows内核来进行I/O的调度，是用于C/S通信模式中性能最好的网络通信模型

IO完成端口充分利用Windows内核对象的调度，只使用少量的几个线程来处理和客户端的所有通信，消除了无谓的线程上下文切换，最大限度的提高了网络通信的性能

注意：
accept()使用阻塞方式接受客户端，同时成千上万的连接会处理不过来；

*/

#include "stdafx.h"

#include <WINSOCK2.H>
#include <Ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

#include <iostream>

#pragma warning(disable:4996)

#define _WINSOCK_DEPRECATED_NO_WARNINGS 0

#define PORT    6700
#define MSGSIZE 1024

using namespace std;

typedef enum
{
	RECV_POSTED
} OPERATION_TYPE;

typedef struct _completionKey
{
	SOCKET client;
	SOCKADDR_IN clientAddr;
} COMPLETIONKEY, *PCOMPLETIONKEY;

typedef struct
{
	WSAOVERLAPPED Overlapped;

	WSABUF         Buff;
	DWORD          NumberOfBytesRecvd;
	DWORD          NumberOfBytesSended;
	DWORD          Flags;

	char           BuffData[MSGSIZE];
	OPERATION_TYPE OperationType;
} PER_IO_OPERATION_DATA, *LPPER_IO_OPERATION_DATA;

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

	//创建工作线程，线程个数是CPU核心的数量 * 2
	SYSTEM_INFO systeminfo;
	GetSystemInfo(&systeminfo);
	for (DWORD i = 0; i < systeminfo.dwNumberOfProcessors * 2; i++)
	{
		CreateThread(NULL, 0, WorkerThread, CompletionPort, 0, NULL);
		cout << "Create Worker Thread " << i+1 << endl;
	}

	//创建服务器SOCKET，指定套接字的类型、使用的协议
	SOCKET sListen;
	sListen = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);

	//绑定端口
	SOCKADDR_IN local;
	local.sin_family = AF_INET;
	local.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	//local.sin_addr.S_un.S_addr = inet_addr("192.168.249.134");
	local.sin_port = htons(PORT);
	bind(sListen, (struct sockaddr *)&local, sizeof(SOCKADDR_IN));
	cout << "bind()" << endl;

	//开始监听，监听队列的最大数量
	listen(sListen, SOMAXCONN);
	cout << "listen()" << endl;

	SOCKET sClient;
	SOCKADDR_IN addrClient;
	int iaddrSize = sizeof(SOCKADDR_IN);

	while (TRUE)
	{
		//接受一个新的客户端Socket
		sClient = accept(sListen, (struct sockaddr *)&addrClient, &iaddrSize);
		printf("Accepted client:%s:%d\n", inet_ntoa(addrClient.sin_addr), ntohs(addrClient.sin_port));

		//将IO完成端口和新的客户端Socket绑定
		PCOMPLETIONKEY pckClient = new COMPLETIONKEY;
		pckClient->client = sClient;
		pckClient->clientAddr = addrClient;
		CreateIoCompletionPort((HANDLE)sClient, CompletionPort, (DWORD)pckClient, 0);	//指定设备句柄、IO完成端口、完成键、线程池线程数量

		//初始化IO操作数据
		LPPER_IO_OPERATION_DATA lpPerIOData = NULL;
		lpPerIOData = (LPPER_IO_OPERATION_DATA)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(PER_IO_OPERATION_DATA));

		lpPerIOData->Buff.len = MSGSIZE;
		lpPerIOData->Buff.buf = lpPerIOData->BuffData;
		lpPerIOData->OperationType = RECV_POSTED;

		//对新的客户端发起异步IO操作 - WSARecv()：发送数据(WSASend和WSASendTo)，接收数据(WSARecv和WSARecvFrom)
		WSARecv(sClient,
			&lpPerIOData->Buff,
			1,
			&lpPerIOData->NumberOfBytesRecvd,
			&lpPerIOData->Flags,
			&lpPerIOData->Overlapped,
			NULL);
	}

	//发送退出消息到队列
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
	HANDLE                  CompletionPort = (HANDLE)CompletionPortID;

	PCOMPLETIONKEY pckClient;
	LPPER_IO_OPERATION_DATA lpPerIOData = NULL;
	DWORD dwBytesTransferred;

	const char* szRespone = "OK";

	while (true)
	{
		//获取队列中处理完成的IO操作
		BOOL bSuccess = GetQueuedCompletionStatus(CompletionPort,
			&dwBytesTransferred,
			(LPDWORD)&pckClient,
			(LPOVERLAPPED *)&lpPerIOData, 
			INFINITE);

		//关闭句柄
		if (FALSE == bSuccess) break;

		//程序退出
		if (0 == pckClient) break;

		//判断完成端口的消息类型
		if (lpPerIOData->OperationType == RECV_POSTED)
		{
			if (dwBytesTransferred == 0)
			{
				//客户端关闭连接
				closesocket(pckClient->client);
				HeapFree(GetProcessHeap(), 0, lpPerIOData);
				cout << "closesocket()" << endl;
			}
			else
			{
				//发送请求响应
				lpPerIOData->BuffData[dwBytesTransferred] = '\0';	//发送一样的信息到客户端
				cout << "Rev() = " << lpPerIOData->BuffData << endl;

				//以阻塞方式发送响应
				send(pckClient->client, szRespone, strlen(szRespone), 0);

				//继续发起异步IO操作 - WSARecv()
				memset(lpPerIOData, 0, sizeof(PER_IO_OPERATION_DATA));
				lpPerIOData->Buff.len = MSGSIZE;
				lpPerIOData->Buff.buf = lpPerIOData->BuffData;
				lpPerIOData->OperationType = RECV_POSTED;

				WSARecv(pckClient->client,
					&lpPerIOData->Buff,
					1,
					&lpPerIOData->NumberOfBytesRecvd,
					&lpPerIOData->Flags,
					&lpPerIOData->Overlapped,
					NULL);
			}
		}
	}

	return 0;
}

