#pragma once

#include "WinSocketDef.h"

#define MSGSIZE 4096

//��������
typedef enum
{
	ACCEPT_POSTED,
	RECV_POSTED,
	SEND_POSTED,
	CLOSE_POSTED,
	QUIT_POSTED
} OPERATION_TYPE;

//��ɼ�
typedef struct
{
	SOCKET ckSocket;
	SOCKADDR_IN ckAddr;
} COMPLETIONKEY, *PCOMPLETIONKEY;

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

LPFN_ACCEPTEX lpfAcceptEx = NULL;
LPFN_GETACCEPTEXSOCKADDRS lpfGetAcceptExSockAddrs = NULL;


