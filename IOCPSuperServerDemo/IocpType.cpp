#include "stdafx.h"
#include "IocpType.h"

IO_OPERATION_DATA::IO_OPERATION_DATA()
{
	reset();

	BuffData = new char[MSGSIZE];
	BuffLen = MSGSIZE;
}

IO_OPERATION_DATA::~IO_OPERATION_DATA()
{
	delete[] BuffData;

	reset();
}

void IO_OPERATION_DATA::reset()
{
	ZeroMemory(this, sizeof(IO_OPERATION_DATA));
}

void IO_OPERATION_DATA::resetBuff()
{
	memset(this->BuffData, 0, this->BuffLen);
}

void IO_OPERATION_DATA::resetOverlapped()
{
	ZeroMemory(&Overlapped, sizeof(OVERLAPPED));

}

const char * COMPLETION_KEY::toString()
{
	sprintf(mAddrInfo, "%s:%d", inet_ntoa(Addr.sin_addr), ntohs(Addr.sin_port));

	return mAddrInfo;
}
