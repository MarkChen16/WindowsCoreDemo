#pragma once

#include "WinSocketDef.h"

#define MSGSIZE 4096

//操作类型
typedef enum
{
	NULL_POSTED,
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

class COMPLETION_KEY
{
public:
	SOCKET Socket;
	SOCKADDR_IN Addr;

	const char *toString();
private:
	char mAddrInfo[100];
};

//IO操作数据
class IO_OPERATION_DATA
{
public:
	IO_OPERATION_DATA();
	~IO_OPERATION_DATA();

	WSAOVERLAPPED  Overlapped;

	WSABUF Buff;
	DWORD NumberOfBytesRecvd;
	DWORD NumberOfBytesSended;
	DWORD Flags;

	char *BuffData;
	DWORD BuffLen;

	SOCKET NewClient;

	OPERATION_TYPE OperationType;

	void reset();
	void resetBuff();
	void resetOverlapped();

private:
	
};

typedef IO_OPERATION_DATA * LPIO_OPERATION_DATA;

