// FileMappingDemo.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"

#include <Windows.h>
#include <iostream>

using namespace std;


/*
�����ڴ�VS�ڴ�ӳ���ļ���
�����ڴ棺Ԥ��һ���ַ�ռ䣬�������������ڴ棨��ϵͳ�������ڴ治��ʱ���浽ҳ�����ļ�����
�ڴ�ӳ���ļ���Ԥ��һ���ַ�ռ䣬�����������������е��ļ���

�ڴ�ӳ���ļ�����;��
1������EXE�ļ�����������ʱ�䣻
2����ȡ����2G���ļ�����������ݽ��л���
3����ͬ����֮�乲������(��ײ�Ļ��ƣ�Ч�����)

*/


//ʹ��dumpbin /Headers xxx.exe����鿴EXE�ļ���PEͷ�͸����ڵ�
//��ͬһ��ִ���ļ��Ĳ�ͬʵ��֮�乲������
#pragma data_seg(".Shared")
volatile int InstanceCount = 1;
#pragma data_seg()

#pragma comment(linker,"/Section:.Shared,RWS")

//��Ԫ�����ݣ�ҳ��С4K�ı���
struct CELL_DATA
{
	//����ҳ�Ĵ�С4K
	union
	{
		char szData[1024];
		char szReserve[4096 - 1];
	};

	bool IsUsed;
};

//��Ч��ȡ���ļ�
void ReadBigFile();

//��ͬ����ͬ������
void ShareData();

//����ϡ���ڴ�ӳ���ļ���ֻ��ʹ��ҳ�ļ���Ϊ�洢��
void CreateSparseFile();

int main()
{
	//ͬһ��EXE�ļ��Ĳ�ͬʵ����������
	cout << "Instance ID: " << InstanceCount++ << endl;

	//ʹ���ڴ�ӳ���ȡ���ļ�
	//ReadBigFile();

	//��������ҳ�ļ����ڴ�ӳ���ļ�
	ShareData();

	//����ϡ����ڴ�ӳ���ļ�
	//CreateSparseFile();

	getchar();
    return 0;
}

//��ȡ���ļ�
void ReadBigFile()
{
	SYSTEM_INFO info;
	GetSystemInfo(&info);

	//���ļ�����
	HANDLE hFile = CreateFile(
		L"D:\\BigFile.zip",
		GENERIC_READ,
		FILE_SHARE_READ, NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_ARCHIVE,
		NULL);

	if (NULL == hFile)
	{
		cout << "Can't open the big file." << endl;
		return;
	}

	//��������Ӳ���ļ����ڴ�ӳ���ļ�
	HANDLE hFileMapping = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);

	//��ȡ�ļ���С
	DWORD dwFileSizeHigh = 0;
	__int64 qwFileSize = GetFileSize(hFile, &dwFileSizeHigh);	//���ص�32λ��С��ͨ�������ٻ�ȡ��32λ��С��������Ӿ��Ǵ��ļ��Ĵ�С
	qwFileSize += (((__int64)dwFileSizeHigh) << 32);

	//�ȹر��ļ������������ļ�������ڴ�ӳ���ļ���������ü���
	CloseHandle(hFile);

	//��ʼ��ȡ���ļ�
	__int64 qwFileOffset = 0, qwNumOfCharA = 0, qwBlockCount = 0;

	while (qwFileSize > 0)
	{
		//ÿ�ζ�ȡ64K
		DWORD dwBytesInBlock = info.dwAllocationGranularity;

		//�������ļ����
		if (qwFileSize < info.dwAllocationGranularity) dwBytesInBlock = (DWORD)qwFileSize;

		//ӳ�䵽���̿ռ�
		PBYTE pbFile = (PBYTE)MapViewOfFile(
			hFileMapping,
			FILE_MAP_READ,
			(DWORD)(qwFileOffset >> 32),
			(DWORD)(qwFileOffset & 0xFFFFFFFF),
			dwBytesInBlock);

		//��ʼ��ȡ���ݣ������ַ�A������
		for (DWORD dwByte = 0; dwByte < dwBytesInBlock; dwByte++)
		{
			if (pbFile[dwByte] == 'A') qwNumOfCharA++;
		}

		//�ر�ӳ��
		UnmapViewOfFile(pbFile);

		qwBlockCount++;
		printf("Block(%I64d): %I64d\n", qwBlockCount, qwNumOfCharA);

		qwFileOffset += dwBytesInBlock;
		qwFileSize -= dwBytesInBlock;
	}

	//�ر��ڴ�ӳ���ļ�
	CloseHandle(hFileMapping);

	cout << "Num of Char A: " << qwNumOfCharA << endl;
}

//��ͬ����֮�乲������
void ShareData()
{
	HANDLE hPageMap = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 64 * 1024, L"GuiQuan-Map-Page");
	if (hPageMap != NULL)
	{
		if (GetLastError() == ERROR_ALREADY_EXISTS)
		{
			//�����ߣ���ȡ����
			PBYTE pbData = (PBYTE)MapViewOfFile(hPageMap, FILE_MAP_READ, 0, 0, 0);

			cout << "Read: " << (char *)pbData << endl;

			//�ر�ӳ��
			UnmapViewOfFile(pbData);
		}
		else if (GetLastError() == NO_ERROR)
		{
			//�����ߣ�д������
			PBYTE pbData = (PBYTE)MapViewOfFile(hPageMap, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);

			memcpy(pbData, "Hello\0", 6);

			cout << "Write: " << pbData << endl;

			//�ر�֮�������߽��̽��ղ�������
			getchar();

			//�ر�ӳ��
			UnmapViewOfFile(pbData);
		}

		CloseHandle(hPageMap);
	}
}

//����ϡ���ļ�
void CreateSparseFile()
{
	HANDLE hFileMapping = CreateFileMapping(
		INVALID_HANDLE_VALUE, 
		NULL, 
		PAGE_READWRITE | SEC_RESERVE, 
		0, 
		100 * sizeof(CELL_DATA), 
		L"GuiQuan-Spa-File");

	//ӳ�䵽���̣�Ԥ��һ�������ַ�ռ�
	PBYTE pbData = (PBYTE)MapViewOfFile(hFileMapping, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);

	//��������洢��
	VirtualAlloc(pbData, 100 * sizeof(CELL_DATA), MEM_COMMIT, PAGE_READWRITE);

	//�������ݣ���������
	pbData[0] = 'A';

	//�ر�ӳ��
	UnmapViewOfFile(pbData);

	CloseHandle(hFileMapping);
}