// ThreadDemo.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"

//Windows API头文件
#include <Windows.h>

#include <iostream>
#include<vector>

using namespace std;

/*
线程管理：
CreateThread、SuspendThread、ResumeThread、TerminateThread、CloseHandle

等待对象的激发状态(Event有信号，Mutex无占用，Semaphore信号量大于零，Process结束，Thread结束)
WaitForSingleObject

临界CriticalSection：
InitializeCriticalSection、DeleteCriticalSection
EnterCriticalSection、LeaveCriticalSection

互斥量Mutex：
CreateMutex、WaitForSingleObject、ReleaseMutex、CloseHandle

事件Event：
CreateEvent、WaitForSingleObject、SetEvent、ResetEvent、CloseHandle

信号量Semaphore：
CreateSemaphore、OpenSemaphore、WaitForSingleObject、ReleaseSemaphore、CloseHandle

*/

//使用互斥量同步
HANDLE hMutex = NULL;
int iTicket = 1000;

//使用临界同步
CRITICAL_SECTION csShowTime;
int iShowTimes = 0;

//事件唤醒其他线程
HANDLE hEvent;

//信号量同步
HANDLE hsemEmpty, hsemFull;
vector<int> buff1;
vector<int> buff2;

vector<int> *currBuff = &buff1;


DWORD WINAPI Fun1Pro(LPVOID lpParameter)
{
	int intSleepSecend = *(int *)lpParameter;

	while (true)
	{
		//临界同步
		EnterCriticalSection(&csShowTime);
		iShowTimes++;
		if (iShowTimes >= 10) SetEvent(hEvent);	//当显示10次后，给事件设置信号
		LeaveCriticalSection(&csShowTime);

		//互斥量同步
		WaitForSingleObject(hMutex, INFINITE);

		if (iTicket > 0)
		{
			cout << "Thread 1: " << iTicket-- << endl;

			Sleep(intSleepSecend);
		}
		else
		{
			break;
		}

		ReleaseMutex(hMutex);
	}

	return 0;
}

DWORD WINAPI Fun2Pro(LPVOID lpParameter)
{
	int intSleepSecend = *(int *)lpParameter;

	while (true)
	{
		//临界同步
		EnterCriticalSection(&csShowTime);
		iShowTimes++;
		if (iShowTimes >= 10) SetEvent(hEvent);	//当显示10次后，给事件设置信号
		LeaveCriticalSection(&csShowTime);

		//互斥量同步
		WaitForSingleObject(hMutex, INFINITE);

		if (iTicket > 0)
		{
			cout << "Thread 2: " << iTicket-- << endl;

			Sleep(intSleepSecend);
		}
		else
		{
			break;
		}

		ReleaseMutex(hMutex);

		//信号量同步-产生数据
		if (currBuff->size() < 10)
		{
			currBuff->push_back(iTicket);
		}
		else
		{
			if (WaitForSingleObject(hsemEmpty, 10) == 0)	//获取一个空的缓存，信号量减一
			{
				if (currBuff == &buff1) currBuff = &buff2;
				else currBuff = &buff1;

				ReleaseSemaphore(hsemFull, 1, NULL);	//释放一个满的缓存，信号量加一

				currBuff->push_back(iTicket);
			}
		}
	}

	return 0;
}

DWORD WINAPI FunShowTime(LPVOID lpParameter)
{
	//事件同步
	WaitForSingleObject(hEvent, INFINITE);
	cout << "ShowTime >= 10" << endl;
	ResetEvent(hEvent);		//复位信号

	while (true)
	{
		//信号量同步-使用数据
		if (WaitForSingleObject(hsemFull, 1000) == 0)	//获得一个满的缓存
		{
			vector<int> *fullBuff = &buff1;
			if (currBuff == &buff1) fullBuff = &buff2;

			cout << "Buff: ";
			for (vector<int>::iterator itor = fullBuff->begin();  itor != fullBuff->end(); itor++)
			{
				cout << *itor << " ";
			}
			cout << endl;

			fullBuff->clear();	//清空数据

			ReleaseSemaphore(hsemEmpty, 1, NULL);	//释放一个空的缓存

			break;
		}
	}

	return 0;
}

int main()
{
	//只允许一个程序实例同时执行；
	HANDLE hDemoApp = CreateMutex(NULL, false, NULL);
	if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
		MessageBox(NULL, L"只允许一个程序实例同时执行！", L"错误", MB_OK);

		exit(0);
	}

	//创建临界
	InitializeCriticalSection(&csShowTime);

	//创建互斥量
	hMutex = CreateMutex(NULL, false, NULL);

	//创建事件
	hEvent = CreateEvent(NULL, true, false, NULL);	//创建一个手动复位，无信号的事件

	//创建信号量
	hsemEmpty = CreateSemaphore(NULL, 2, 2, NULL);	//创建一个初始2个，最大2个的信号量
	hsemFull = CreateSemaphore(NULL, 0, 2, NULL);	//创建一个初始0个，最大2个的信号量

	int intSleepSecend1 = 1000;
	int intSleepSecend2 = 200;
	HANDLE hThread1 = CreateThread(NULL, 0, Fun1Pro, &intSleepSecend1, CREATE_SUSPENDED, NULL);	//创建一个挂起状态的线程，并传递参数
	HANDLE hThread2 = CreateThread(NULL, 0, Fun2Pro, &intSleepSecend2, 0, NULL);	//创建一个线程立即执行，并传递参数
	HANDLE hThread3 = CreateThread(NULL, 0, FunShowTime, NULL, 0, NULL);

	Sleep(2000);
	ResumeThread(hThread1);		//恢复线程

	Sleep(3000);
	//SuspendThread(hThread1);	//挂起线程(注意有风险，如果获取互斥量还没释放就挂起，容易引起死锁)

	Sleep(5000);
	//TerminateThread(hThread1, 0);	//终止线程(注意有风险，如果获取互斥量还没释放就退出，容易引起死锁)

	//修改数据
	WaitForSingleObject(hMutex, INFINITE);
	iTicket = 3;
	ReleaseMutex(hMutex);

	WaitForSingleObject(hThread1, INFINITE);	//等等线程对象被激发
	WaitForSingleObject(hThread2, INFINITE);
	WaitForSingleObject(hThread3, INFINITE);

	//释放句柄
	CloseHandle(hThread1);
	CloseHandle(hThread2);

	//删除临界
	DeleteCriticalSection(&csShowTime);
	CloseHandle(hMutex);
	CloseHandle(hEvent);

	Sleep(1000);
    return 0;
}

