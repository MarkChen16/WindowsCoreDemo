#include "stdafx.h"
#include "IocpTcpServer.h"


IocpTcpServer::IocpTcpServer()
{

}

IocpTcpServer::~IocpTcpServer()
{

}

void IocpTcpServer::initServer(int intWorkerCount)
{
}

void IocpTcpServer::setMaxConnection(int intMaxCount)
{
}

void IocpTcpServer::setMaxBufferSize(int intMaxSize)
{
}

bool IocpTcpServer::openServer(const char * lpServerIP, int intPort)
{
	return false;
}

void IocpTcpServer::suspendServer()
{
}

void IocpTcpServer::resumeServer()
{
}

void IocpTcpServer::askForStop()
{
}

void IocpTcpServer::waitForStop(int intMsec)
{

}

void IocpTcpServer::closeServer()
{
}

void IocpTcpServer::connectEvent()
{
}

void IocpTcpServer::sendEvent()
{
}

void IocpTcpServer::recvEvent()
{
}

void IocpTcpServer::closeEvent()
{
}

void IocpTcpServer::errorEvent()
{
}
