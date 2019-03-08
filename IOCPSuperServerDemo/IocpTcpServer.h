#pragma once

#include "WinSocketDef.h"
#include "IocpType.h"

#include <vector>

using namespace std;

class IocpTcpServer
{
public:
	IocpTcpServer();
	virtual ~IocpTcpServer();

	void initServer(int intWorkerCount = 0);

	void setMaxConnection(int intMaxCount);

	void setMaxBufferSize(int intMaxSize);

	bool openServer(const char *lpServerIP, int intPort);

	void suspendServer();

	void resumeServer();

	void askForStop();

	void waitForStop(int intMsec = 0);

	void closeServer();


protected:
	virtual void connectEvent();
	virtual void sendEvent();
	virtual void recvEvent();
	virtual void closeEvent();
	virtual void errorEvent();

private:
	//void doAccept();
	//void doSend();
	//void doRecv();
	//void doClose();
	//void doQuit();

private:


};


