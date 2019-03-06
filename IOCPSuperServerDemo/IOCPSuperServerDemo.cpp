// IOCPServerDemo.cpp : �������̨Ӧ�ó������ڵ㡣
//


/*
��ɶ˿ڻ�������Windows�ں�������I/O�ĵ��ȣ�������C/Sͨ��ģʽ��������õ�����ͨ��ģ�ͣ�

ע�⣺
ʹ��AcceptEx()��ʹ����ɶ˿ڵķ�ʽ���ܿͻ��˵����ӣ��÷�����򲢷����ܸ���ǿ��
*/

#include "stdafx.h"

#include <iostream>

#include <WINSOCK2.H>
#include <ws2tcpip.h>

#include <mswsock.h>     //΢����չ�����AcceptEx()

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "Mswsock.lib")

#pragma warning(disable:4996)

using namespace std;

#define PORT    6700
#define MSGSIZE 1024

//��ɼ�
typedef struct
{
	SOCKET ckSocket;
	SOCKADDR_IN ckAddr;
} COMPLETIONKEY, *PCOMPLETIONKEY;

//��������
typedef enum
{
	ACCEPTEX_POSTED,
	RECV_POSTED,
	SEND_POSTED
} OPERATION_TYPE;

//IO��������
typedef struct
{
	WSAOVERLAPPED Overlapped;
	WSABUF         Buff;	//���ݻ�����,����WSASend/WSARecv
	DWORD          NumberOfBytesRecvd;
	DWORD          NumberOfBytesSended;
	DWORD          Flags;

	char           BuffData[MSGSIZE];
	OPERATION_TYPE OperationType;

	SOCKET newClient;
} PER_IO_OPERATION_DATA, *LPPER_IO_OPERATION_DATA;

//��������������չ��ģ�ͨ������ָ����ã�����ÿ�ζ�ȥ����WSAIoctl
LPFN_ACCEPTEX lpfAcceptEx = NULL;
LPFN_GETACCEPTEXSOCKADDRS lpfGetAcceptExSockAddrs = NULL;

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

	//�����������̣߳��̸߳�����CPU���ĵ�����
	SYSTEM_INFO systeminfo;
	GetSystemInfo(&systeminfo);
	for (DWORD i = 0; i < systeminfo.dwNumberOfProcessors * 2; i++)
	{
		CreateThread(NULL, 0, WorkerThread, CompletionPort, 0, NULL);
		cout << "Create Worker Thread " << i + 1 << endl;
	}

	//����IO�ص���SOCKET��ָ���׽��ֵ����͡�ʹ�õ�Э��
	SOCKET sListen = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);

	//���AcceptEx��GetAcceptExSockaddrs����ָ��
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

	//�󶨶˿�
	SOCKADDR_IN localAddr;
	localAddr.sin_family = AF_INET;
	localAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	//localAddr.sin_addr.S_un.S_addr = inet_addr("192.168.249.134");
	localAddr.sin_port = htons(PORT);
	bind(sListen, (struct sockaddr *)&localAddr, sizeof(SOCKADDR_IN));
	cout << "bind()" << endl;

	//��IO��ɶ˿ںͼ���Socket�󶨣�����AcceptEx�ص�IO����
	PCOMPLETIONKEY pckServer = new COMPLETIONKEY;
	pckServer->ckSocket = sListen;
	pckServer->ckAddr = localAddr;
	CreateIoCompletionPort((HANDLE)sListen, CompletionPort, (DWORD)pckServer, 0);

	//��ʼ�������������е��������
	listen(sListen, SOMAXCONN);
	cout << "listen()" << endl;

	//Ԥ��N���첽����
	for (int i = 0; i < systeminfo.dwNumberOfProcessors * 5; i++)
	{
		//�첽���ӿͻ��ˣ�����ȡ���ݣ�������ϱ��ص�ַ��Զ�̵�ַ
		LPPER_IO_OPERATION_DATA lpPerIOData = NULL;
		lpPerIOData = (LPPER_IO_OPERATION_DATA)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(PER_IO_OPERATION_DATA));

		lpPerIOData->Buff.len = MSGSIZE;
		lpPerIOData->Buff.buf = lpPerIOData->BuffData;
		lpPerIOData->newClient = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);	//�ȴ���Socket����ʡʱ��
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

	//�ȴ������˳�����
	int i = 0;
	cin >> i;

	//Ͷ���˳���Ϣ������
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
	HANDLE CompletionPort = (HANDLE)CompletionPortID;

	//��ɼ�
	PCOMPLETIONKEY pckData = NULL;

	//IO��������
	LPPER_IO_OPERATION_DATA pioData = NULL;

	DWORD dwBytesTransferred;
	const char* szRespone = "OK";

	while (true)
	{
		//��ȡ�����д�����ɵ�IO����
		BOOL bSuccess = GetQueuedCompletionStatus(CompletionPort,
			&dwBytesTransferred,
			(LPDWORD)&pckData,
			(LPOVERLAPPED *)&pioData,
			INFINITE);

		//�رվ��
		if (FALSE == bSuccess) break;

		//�����˳�
		if (0 == pckData) break;

		//�ж���ɶ˿ڵ���Ϣ����
		if (ACCEPTEX_POSTED == pioData->OperationType)
		{
			//����AcceptEx()�����Ľ��======================================
			//��ȡ���ݣ����ص�ַ��Զ�̵�ַ
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

			//��������AcceptEx���������ͻ��˵�����
			ZeroMemory(pioData, sizeof(pioData));
			pioData->Buff.len = MSGSIZE;
			pioData->Buff.buf = pioData->BuffData;
			pioData->newClient = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);	//�ȴ���Socket����ʡʱ��
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

			//�������󣬷�����Ӧ
			printf("New Client[%s#%d]: %s\n", inet_ntoa(peerAddr.sin_addr), ntohs(peerAddr.sin_port), recvData);

			send(mClient, szRespone, strlen(szRespone), 0);

			//��IO��ɶ˿ں��µĿͻ���Socket��
			PCOMPLETIONKEY pckClient = new COMPLETIONKEY;
			pckClient->ckSocket = mClient;
			pckClient->ckAddr = peerAddr;
			CreateIoCompletionPort((HANDLE)pckClient->ckSocket, CompletionPort, (DWORD)pckClient, 0);

			//���µĿͻ��˷����첽IO���� - WSARecv()����������(WSASend��WSASendTo)����������(WSARecv��WSARecvFrom)
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
			//����WSARecv()�����Ľ��======================================
			if (dwBytesTransferred == 0)
			{
				//�ͻ��˹ر�����
				closesocket(pckData->ckSocket);
				HeapFree(GetProcessHeap(), 0, pioData);
				//cout << "closesocket()" << endl;
			}
			else
			{
				//����������Ӧ
				pioData->BuffData[dwBytesTransferred] = '\0';	//����һ������Ϣ���ͻ���
				cout << "Rev() = " << pioData->BuffData << endl;

				//��������ʽ������Ӧ
				send(pckData->ckSocket, szRespone, strlen(szRespone), 0);

				//���������첽IO���� - WSARecv()
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

