// ThreadDemo.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"

//Windows APIͷ�ļ�
#include <Windows.h>

#include <iostream>
#include<vector>

using namespace std;

/*
�̹߳���
CreateThread��SuspendThread��ResumeThread��TerminateThread��CloseHandle

�ȴ�����ļ���״̬(Event���źţ�Mutex��ռ�ã�Semaphore�ź��������㣬Process������Thread����)
WaitForSingleObject

�ٽ�CriticalSection��
InitializeCriticalSection��DeleteCriticalSection
EnterCriticalSection��LeaveCriticalSection

������Mutex��
CreateMutex��WaitForSingleObject��ReleaseMutex��CloseHandle

�¼�Event��
CreateEvent��WaitForSingleObject��SetEvent��ResetEvent��CloseHandle

�ź���Semaphore��
CreateSemaphore��OpenSemaphore��WaitForSingleObject��ReleaseSemaphore��CloseHandle

*/

//ʹ�û�����ͬ��
HANDLE hMutex = NULL;
int iTicket = 1000;

//ʹ���ٽ�ͬ��
CRITICAL_SECTION csShowTime;
int iShowTimes = 0;

//�¼����������߳�
HANDLE hEvent;

//�ź���ͬ��
HANDLE hsemEmpty, hsemFull;
vector<int> buff1;
vector<int> buff2;

vector<int> *currBuff = &buff1;


DWORD WINAPI Fun1Pro(LPVOID lpParameter)
{
	int intSleepSecend = *(int *)lpParameter;

	while (true)
	{
		//�ٽ�ͬ��
		EnterCriticalSection(&csShowTime);
		iShowTimes++;
		if (iShowTimes >= 10) SetEvent(hEvent);	//����ʾ10�κ󣬸��¼������ź�
		LeaveCriticalSection(&csShowTime);

		//������ͬ��
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
		//�ٽ�ͬ��
		EnterCriticalSection(&csShowTime);
		iShowTimes++;
		if (iShowTimes >= 10) SetEvent(hEvent);	//����ʾ10�κ󣬸��¼������ź�
		LeaveCriticalSection(&csShowTime);

		//������ͬ��
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

		//�ź���ͬ��-��������
		if (currBuff->size() < 10)
		{
			currBuff->push_back(iTicket);
		}
		else
		{
			if (WaitForSingleObject(hsemEmpty, 10) == 0)	//��ȡһ���յĻ��棬�ź�����һ
			{
				if (currBuff == &buff1) currBuff = &buff2;
				else currBuff = &buff1;

				ReleaseSemaphore(hsemFull, 1, NULL);	//�ͷ�һ�����Ļ��棬�ź�����һ

				currBuff->push_back(iTicket);
			}
		}
	}

	return 0;
}

DWORD WINAPI FunShowTime(LPVOID lpParameter)
{
	//�¼�ͬ��
	WaitForSingleObject(hEvent, INFINITE);
	cout << "ShowTime >= 10" << endl;
	ResetEvent(hEvent);		//��λ�ź�

	while (true)
	{
		//�ź���ͬ��-ʹ������
		if (WaitForSingleObject(hsemFull, 1000) == 0)	//���һ�����Ļ���
		{
			vector<int> *fullBuff = &buff1;
			if (currBuff == &buff1) fullBuff = &buff2;

			cout << "Buff: ";
			for (vector<int>::iterator itor = fullBuff->begin();  itor != fullBuff->end(); itor++)
			{
				cout << *itor << " ";
			}
			cout << endl;

			fullBuff->clear();	//�������

			ReleaseSemaphore(hsemEmpty, 1, NULL);	//�ͷ�һ���յĻ���

			break;
		}
	}

	return 0;
}

int main()
{
	//ֻ����һ������ʵ��ͬʱִ�У�
	HANDLE hDemoApp = CreateMutex(NULL, false, NULL);
	if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
		MessageBox(NULL, L"ֻ����һ������ʵ��ͬʱִ�У�", L"����", MB_OK);

		exit(0);
	}

	//�����ٽ�
	InitializeCriticalSection(&csShowTime);

	//����������
	hMutex = CreateMutex(NULL, false, NULL);

	//�����¼�
	hEvent = CreateEvent(NULL, true, false, NULL);	//����һ���ֶ���λ�����źŵ��¼�

	//�����ź���
	hsemEmpty = CreateSemaphore(NULL, 2, 2, NULL);	//����һ����ʼ2�������2�����ź���
	hsemFull = CreateSemaphore(NULL, 0, 2, NULL);	//����һ����ʼ0�������2�����ź���

	int intSleepSecend1 = 1000;
	int intSleepSecend2 = 200;
	HANDLE hThread1 = CreateThread(NULL, 0, Fun1Pro, &intSleepSecend1, CREATE_SUSPENDED, NULL);	//����һ������״̬���̣߳������ݲ���
	HANDLE hThread2 = CreateThread(NULL, 0, Fun2Pro, &intSleepSecend2, 0, NULL);	//����һ���߳�����ִ�У������ݲ���
	HANDLE hThread3 = CreateThread(NULL, 0, FunShowTime, NULL, 0, NULL);

	Sleep(2000);
	ResumeThread(hThread1);		//�ָ��߳�

	Sleep(3000);
	//SuspendThread(hThread1);	//�����߳�(ע���з��գ������ȡ��������û�ͷž͹���������������)

	Sleep(5000);
	//TerminateThread(hThread1, 0);	//��ֹ�߳�(ע���з��գ������ȡ��������û�ͷž��˳���������������)

	//�޸�����
	WaitForSingleObject(hMutex, INFINITE);
	iTicket = 3;
	ReleaseMutex(hMutex);

	WaitForSingleObject(hThread1, INFINITE);	//�ȵ��̶߳��󱻼���
	WaitForSingleObject(hThread2, INFINITE);
	WaitForSingleObject(hThread3, INFINITE);

	//�ͷž��
	CloseHandle(hThread1);
	CloseHandle(hThread2);

	//ɾ���ٽ�
	DeleteCriticalSection(&csShowTime);
	CloseHandle(hMutex);
	CloseHandle(hEvent);

	Sleep(1000);
    return 0;
}

