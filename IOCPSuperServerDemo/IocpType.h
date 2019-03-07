#pragma once

#include "WinSocketDef.h"

#define MSGSIZE 4096

//操作类型
typedef enum
{
	ACCEPT_POSTED,
	RECV_POSTED,
	SEND_POSTED,
	CLOSE_POSTED,
	QUIT_POSTED
} OPERATION_TYPE;

//完成键
typedef struct
{
	SOCKET ckSocket;
	SOCKADDR_IN ckAddr;
} COMPLETIONKEY, *PCOMPLETIONKEY;

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

LPFN_ACCEPTEX lpfAcceptEx = NULL;
LPFN_GETACCEPTEXSOCKADDRS lpfGetAcceptExSockAddrs = NULL;


