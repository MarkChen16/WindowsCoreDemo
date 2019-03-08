// IOCPServerDemo.cpp : �������̨Ӧ�ó������ڵ㡣
//


/*
��ɶ˿ڻ�������Windows�ں�������I/O�ĵ��ȣ�������C/Sͨ��ģʽ��������õ�����ͨ��ģ�ͣ�

ע�⣺
ʹ��AcceptEx()��ʹ����ɶ˿ڵķ�ʽ���ܿͻ��˵����ӣ��÷�����򲢷����ܸ���ǿ��

Windows �µ������TCP������
����ϵͳ�������������������TCP��������Windows �µ�����TCP�������ж��������ͬ������
���¶���ͨ���޸�ע���[HKEY_LOCAL_MACHINE \System \CurrentControlSet \Services \Tcpip \Parameters]
1. ���TCP������             TcpNumConnections
2. TCP�ر��ӳ�ʱ��           TCPTimedWaitDelay    (30-240)s        
3. ���̬�˿���            MaxUserPort  (Default = 5000, Max = 65534) TCP�ͻ��˺ͷ���������ʱ���ͻ��˱������һ����̬�˿ڣ�Ĭ������������̬�˿ڵķ��䷶ΧΪ 1024-5000 ��Ҳ����˵Ĭ������£��ͻ���������ͬʱ����3977 ��Socket ����    
4. ���TCB ����              MaxFreeTcbsϵͳΪÿ��TCP ���ӷ���һ��TCP ���ƿ�(TCP control block or TCB)��������ƿ����ڻ���TCP���ӵ�һЩ������ÿ��TCB��Ҫ���� 0.5 KB��pagepool �� 0.5KB ��Non-pagepool��Ҳ��˵��ÿ��TCP���ӻ�ռ�� 1KB ��ϵͳ�ڴ档��Server�汾��MaxFreeTcbs ��Ĭ��ֵΪ1000 ��64M ���������ڴ棩Server �汾�������Ĭ��ֵΪ 2000��Ҳ����˵��Ĭ������£�Server �汾���ͬʱ���Խ���������2000��TCP ���ӡ�
5. ���TCB Hash table ����   MaxHashTableSize TCB ��ͨ��Hash table ������ġ����ֵָ������ pagepool �ڴ��������Ҳ����˵�����MaxFreeTcbs = 1000 , �� pagepool ���ڴ�����Ϊ 500KB��ô MaxHashTableSize Ӧ���� 500 ���С��������Խ����Hash table ������Ⱦ�Խ�ߣ�ÿ�η���Ͳ��� TCP  ������ʱ��Խ�١����ֵ������2���ݣ������Ϊ65536.
*/

#include "stdafx.h"

#include <iostream>

#include <vector>

#include <WINSOCK2.H>
#include <ws2tcpip.h>
#include <mswsock.h>     //΢����չ�����AcceptEx()

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "Mswsock.lib")

#pragma warning(disable:4996)

using namespace std;

//�����Ķ˿�
#define PORT    6700

//����ʹ��4K��һ���ڴ�ҳ���С
#define MSGSIZE 4096 * 10

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

	//�����������̣߳��̸߳�����CPU���ĵ���������
	SYSTEM_INFO systeminfo;
	GetSystemInfo(&systeminfo);
	int cntWorker = systeminfo.dwNumberOfProcessors * 2 + 2;

	for (int i = 0; i < cntWorker; i++)
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
	localAddr.sin_addr.S_un.S_addr = inet_addr("192.168.249.134");
	localAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

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

	//Ͷ��N���첽���Ӳ�������ϵͳ���ӿͻ��˲���ȡ��һ�����ݺ���ӵ���ɶ˿ڶ����У�
	cout << "Max Connection: ";
	int cntMaxConnection = 0;
	cin >> cntMaxConnection;

	vector<LPPER_IO_OPERATION_DATA> lstAcceptIOData;
	for (int i = 0; i < cntMaxConnection; i++)
	{
		//�첽���ӿͻ��ˣ�����ȡ���ݣ�������ϱ��ص�ַ��Զ�̵�ַ
		LPPER_IO_OPERATION_DATA lpPerIOData = NULL;
		lpPerIOData = (LPPER_IO_OPERATION_DATA)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(PER_IO_OPERATION_DATA));
		lstAcceptIOData.push_back(lpPerIOData);

		lpPerIOData->Buff.len = MSGSIZE;
		lpPerIOData->Buff.buf = lpPerIOData->BuffData;
		lpPerIOData->OperationType = ACCEPTEX_POSTED;

		//�ȴ���Socket����ʡʱ��
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

	//�ȴ������˳�����
	int i = 0;
	std::cin >> i;

	//Ͷ���˳���Ϣ������
	for (int i = 0; i < cntWorker; i++)
	{
		PostQueuedCompletionStatus(CompletionPort, 0, 0, NULL);
	}

	//�رվ��
	CloseHandle(CompletionPort);

	//�رշ����Socket
	closesocket(sListen);
	WSACleanup();

	//���AcceptEx��IO��������
	for each (auto var in lstAcceptIOData)
	{
		HeapFree(GetProcessHeap(), 1, var);
	}
	lstAcceptIOData.clear();

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
			send(mClient, szRespone, strlen(szRespone), 0);

			//��IO��ɶ˿ں��µĿͻ���Socket��
			PCOMPLETIONKEY pckClient = new COMPLETIONKEY;
			pckClient->ckSocket = mClient;
			pckClient->ckAddr = clientAddr;
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

