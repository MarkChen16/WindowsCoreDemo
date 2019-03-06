// IOCPServerDemo.cpp : �������̨Ӧ�ó������ڵ㡣
//


/*
��ɶ˿ڻ�������Windows�ں�������I/O�ĵ��ȣ�������C/Sͨ��ģʽ��������õ�����ͨ��ģ��

IO��ɶ˿ڳ������Windows�ں˶���ĵ��ȣ�ֻʹ�������ļ����߳�������Ϳͻ��˵�����ͨ�ţ���������ν���߳��������л�������޶ȵ����������ͨ�ŵ�����

ע�⣺
accept()ʹ��������ʽ���ܿͻ��ˣ�ͬʱ��ǧ��������ӻᴦ��������

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

//�������߳�
DWORD WINAPI WorkerThread(LPVOID);

int main()
{
	//��ʼ��SOCKET�⣬���ð汾
	WSADATA wsaData;
	WSAStartup(0x0202, &wsaData);

	//����IO��ɶ˿�
	HANDLE CompletionPort = INVALID_HANDLE_VALUE;
	CompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

	//���������̣߳��̸߳�����CPU���ĵ����� * 2
	SYSTEM_INFO systeminfo;
	GetSystemInfo(&systeminfo);
	for (DWORD i = 0; i < systeminfo.dwNumberOfProcessors * 2; i++)
	{
		CreateThread(NULL, 0, WorkerThread, CompletionPort, 0, NULL);
		cout << "Create Worker Thread " << i+1 << endl;
	}

	//����������SOCKET��ָ���׽��ֵ����͡�ʹ�õ�Э��
	SOCKET sListen;
	sListen = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);

	//�󶨶˿�
	SOCKADDR_IN local;
	local.sin_family = AF_INET;
	local.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	//local.sin_addr.S_un.S_addr = inet_addr("192.168.249.134");
	local.sin_port = htons(PORT);
	bind(sListen, (struct sockaddr *)&local, sizeof(SOCKADDR_IN));
	cout << "bind()" << endl;

	//��ʼ�������������е��������
	listen(sListen, SOMAXCONN);
	cout << "listen()" << endl;

	SOCKET sClient;
	SOCKADDR_IN addrClient;
	int iaddrSize = sizeof(SOCKADDR_IN);

	while (TRUE)
	{
		//����һ���µĿͻ���Socket
		sClient = accept(sListen, (struct sockaddr *)&addrClient, &iaddrSize);
		printf("Accepted client:%s:%d\n", inet_ntoa(addrClient.sin_addr), ntohs(addrClient.sin_port));

		//��IO��ɶ˿ں��µĿͻ���Socket��
		PCOMPLETIONKEY pckClient = new COMPLETIONKEY;
		pckClient->client = sClient;
		pckClient->clientAddr = addrClient;
		CreateIoCompletionPort((HANDLE)sClient, CompletionPort, (DWORD)pckClient, 0);	//ָ���豸�����IO��ɶ˿ڡ���ɼ����̳߳��߳�����

		//��ʼ��IO��������
		LPPER_IO_OPERATION_DATA lpPerIOData = NULL;
		lpPerIOData = (LPPER_IO_OPERATION_DATA)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(PER_IO_OPERATION_DATA));

		lpPerIOData->Buff.len = MSGSIZE;
		lpPerIOData->Buff.buf = lpPerIOData->BuffData;
		lpPerIOData->OperationType = RECV_POSTED;

		//���µĿͻ��˷����첽IO���� - WSARecv()����������(WSASend��WSASendTo)����������(WSARecv��WSARecvFrom)
		WSARecv(sClient,
			&lpPerIOData->Buff,
			1,
			&lpPerIOData->NumberOfBytesRecvd,
			&lpPerIOData->Flags,
			&lpPerIOData->Overlapped,
			NULL);
	}

	//�����˳���Ϣ������
	for (DWORD i = 0; i < systeminfo.dwNumberOfProcessors * 2; i++)
	{
		PostQueuedCompletionStatus(CompletionPort, 0, 0, NULL);
	}

	//�رվ��
	CloseHandle(CompletionPort);

	//�رշ����Socket
	closesocket(sListen);
	WSACleanup();

	return 0;
}

DWORD WINAPI WorkerThread(LPVOID CompletionPortID)
{
	//IO��ɶ˿�
	HANDLE                  CompletionPort = (HANDLE)CompletionPortID;

	PCOMPLETIONKEY pckClient;
	LPPER_IO_OPERATION_DATA lpPerIOData = NULL;
	DWORD dwBytesTransferred;

	const char* szRespone = "OK";

	while (true)
	{
		//��ȡ�����д�����ɵ�IO����
		BOOL bSuccess = GetQueuedCompletionStatus(CompletionPort,
			&dwBytesTransferred,
			(LPDWORD)&pckClient,
			(LPOVERLAPPED *)&lpPerIOData, 
			INFINITE);

		//�رվ��
		if (FALSE == bSuccess) break;

		//�����˳�
		if (0 == pckClient) break;

		//�ж���ɶ˿ڵ���Ϣ����
		if (lpPerIOData->OperationType == RECV_POSTED)
		{
			if (dwBytesTransferred == 0)
			{
				//�ͻ��˹ر�����
				closesocket(pckClient->client);
				HeapFree(GetProcessHeap(), 0, lpPerIOData);
				cout << "closesocket()" << endl;
			}
			else
			{
				//����������Ӧ
				lpPerIOData->BuffData[dwBytesTransferred] = '\0';	//����һ������Ϣ���ͻ���
				cout << "Rev() = " << lpPerIOData->BuffData << endl;

				//��������ʽ������Ӧ
				send(pckClient->client, szRespone, strlen(szRespone), 0);

				//���������첽IO���� - WSARecv()
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

