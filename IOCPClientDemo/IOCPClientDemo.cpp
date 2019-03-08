// IOCPClientDemo.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"

#include <WINSOCK2.H>
#include <ws2tcpip.h>
#include <mswsock.h>     //微软扩展的类库AcceptEx()

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "Mswsock.lib")

#include <iostream>
#include <vector>
#include <string>

using namespace std;

class MutexLocker
{
public:
	MutexLocker(HANDLE &mutex) : mMutex(mutex)
	{
		dwResult = WaitForSingleObject(mMutex, INFINITE);
	}

	virtual ~MutexLocker()
	{
		if (0 == dwResult) ReleaseMutex(mMutex);
	}

private:
	HANDLE &mMutex;
	DWORD dwResult;
};

class BaseThread
{
public:
	BaseThread()
	{
		mThread = CreateThread(NULL, 1, BaseThread::runThreadFun, this, CREATE_SUSPENDED, &mThreadID);
	}

	virtual ~BaseThread()
	{
		CloseHandle(mThread);
	}

	DWORD start()
	{
		return resume();
	}

	DWORD suspend()
	{
		return SuspendThread(mThread);
	}

	DWORD resume()
	{
		return ResumeThread(mThread);
	}

	DWORD wait(DWORD dwMsec = 0)
	{
		if (0 == dwMsec) return WaitForSingleObject(mThread, INFINITE);
		else return WaitForSingleObject(mThread, dwMsec);
	}

	BOOL terminate()
	{
		return TerminateThread(mThread, 0);
	}

	DWORD getID()
	{
		return mThreadID;
	}

protected:
	virtual void run() = 0;

private:
	static DWORD WINAPI runThreadFun(LPVOID lpParam)
	{
		BaseThread *pThis = (BaseThread *)lpParam;
		pThis->run();

		return 0;
	}

	DWORD mThreadID = 0;
	HANDLE mThread = NULL;
};

class ClientThread : public BaseThread
{
public:
	ClientThread(int intMsec = 1000)
	{
		mMutex = CreateMutex(NULL, false, NULL);

		iIntervalMsec = intMsec;
	}

	virtual ~ClientThread()
	{
		CloseHandle(mMutex);
	}

	void askforPause(bool bValue)
	{
		MutexLocker locker(mMutex);
		mPause = bValue;
	}

	void askforStop()
	{
		MutexLocker locker(mMutex);
		mStop = true;
	}

protected:
	void run() override
	{
		WORD sockVersion = MAKEWORD(2, 2);

		WSADATA data;
		WSAStartup(sockVersion, &data);

		char ucBuff[4096];
		int lenBuff = 4096;

		while (true)
		{
			Sleep(iIntervalMsec);

			//判断是否暂停和退出
			{
				MutexLocker locker(mMutex);
				if (true == mStop) break;

				if (true == mPause) continue;
			}
			
			//连接服务器
			SOCKET sClient = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			if (sClient == INVALID_SOCKET)
			{
				cout << "C";
				continue;
			}

			sockaddr_in serAddr;
			serAddr.sin_family = AF_INET;
			serAddr.sin_port = htons(6700);
			serAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
			serAddr.sin_addr.S_un.S_addr = inet_addr("192.168.249.134");
			if (connect(sClient, (sockaddr *)&serAddr, sizeof(serAddr)) == SOCKET_ERROR)
			{
				closesocket(sClient);

				cout << "E";
				continue;
			}
			
			//发送数据
			memset(ucBuff, 0x00, sizeof(ucBuff));
			memcpy(ucBuff, "123", 3);
			int intSend = send(sClient, ucBuff, 4, 0);

			//接收数据
			memset(ucBuff, 0x00, sizeof(ucBuff));
			int intRev = recv(sClient, ucBuff, lenBuff, 0);

			//printf("%d Thread: %s\n", getID(), ucBuff);

			closesocket(sClient);
		}

		WSACleanup();

		ExitThread(0);
	}

private:
	HANDLE mMutex = NULL;
	bool mPause = false;
	bool mStop = false;

	int iIntervalMsec = 1000; 
};

int main()
{
	int intThreadCount = 1000, intMsec = 1000, intTmp = 0;

	//cin >> intThreadCount >> intMsec;

	//创建并开启线程
	vector<ClientThread *> lstThread;

	for (int i = 0; i < intThreadCount; i++)
	{
		ClientThread *newClient = new ClientThread(intMsec);
		newClient->start();

		lstThread.push_back(newClient);
	}

	cin >> intTmp;

	//停止线程并删除线程
	for (vector<ClientThread *>::iterator itor = lstThread.begin(); itor != lstThread.end(); itor++)
	{
		(*itor)->askforStop();
	}

	for (vector<ClientThread *>::iterator itor = lstThread.begin(); itor != lstThread.end(); itor++)
	{
		(*itor)->wait();
		delete *itor;
	}

	lstThread.clear();

	return 0;
}

