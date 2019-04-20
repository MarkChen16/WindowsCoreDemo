// FileMappingDemo.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"

#include <Windows.h>
#include <iostream>

using namespace std;


/*
虚拟内存VS内存映射文件：
虚拟内存：预订一块地址空间，并调拨到物理内存（当系统的物理内存不足时保存到页交换文件）；
内存映射文件：预订一块地址空间，并调拨到磁盘上已有的文件；

内存映射文件的用途：
1、加载EXE文件，减少启动时间；
2、读取大于2G的文件，避免对数据进行缓存
3、不同进程之间共享数据(最底层的机制，效率最高)

*/


//使用dumpbin /Headers xxx.exe命令查看EXE文件的PE头和各个节点
//在同一个执行文件的不同实例之间共享数据
#pragma data_seg(".Shared")
volatile int InstanceCount = 1;
#pragma data_seg()

#pragma comment(linker,"/Section:.Shared,RWS")

//单元格数据，页大小4K的倍数
struct CELL_DATA
{
	//对齐页的大小4K
	union
	{
		char szData[1024];
		char szReserve[4096 - 1];
	};

	bool IsUsed;
};

//高效读取大文件
void ReadBigFile();

//不同进程同享数据
void ShareData();

//创建稀疏内存映射文件，只能使用页文件作为存储器
void CreateSparseFile();

int main()
{
	//同一个EXE文件的不同实例共享数据
	cout << "Instance ID: " << InstanceCount++ << endl;

	//使用内存映射读取大文件
	//ReadBigFile();

	//创建基于页文件的内存映射文件
	ShareData();

	//创建稀疏的内存映射文件
	//CreateSparseFile();

	getchar();
    return 0;
}

//读取大文件
void ReadBigFile()
{
	SYSTEM_INFO info;
	GetSystemInfo(&info);

	//打开文件对象
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

	//创建基于硬盘文件的内存映射文件
	HANDLE hFileMapping = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);

	//获取文件大小
	DWORD dwFileSizeHigh = 0;
	__int64 qwFileSize = GetFileSize(hFile, &dwFileSizeHigh);	//返回低32位大小，通过引用再获取高32位大小，两者相加就是大文件的大小
	qwFileSize += (((__int64)dwFileSizeHigh) << 32);

	//先关闭文件，避免增加文件对象和内存映射文件对象的引用计数
	CloseHandle(hFile);

	//开始读取大文件
	__int64 qwFileOffset = 0, qwNumOfCharA = 0, qwBlockCount = 0;

	while (qwFileSize > 0)
	{
		//每次读取64K
		DWORD dwBytesInBlock = info.dwAllocationGranularity;

		//当读到文件最后
		if (qwFileSize < info.dwAllocationGranularity) dwBytesInBlock = (DWORD)qwFileSize;

		//映射到进程空间
		PBYTE pbFile = (PBYTE)MapViewOfFile(
			hFileMapping,
			FILE_MAP_READ,
			(DWORD)(qwFileOffset >> 32),
			(DWORD)(qwFileOffset & 0xFFFFFFFF),
			dwBytesInBlock);

		//开始读取数据，计算字符A的数量
		for (DWORD dwByte = 0; dwByte < dwBytesInBlock; dwByte++)
		{
			if (pbFile[dwByte] == 'A') qwNumOfCharA++;
		}

		//关闭映射
		UnmapViewOfFile(pbFile);

		qwBlockCount++;
		printf("Block(%I64d): %I64d\n", qwBlockCount, qwNumOfCharA);

		qwFileOffset += dwBytesInBlock;
		qwFileSize -= dwBytesInBlock;
	}

	//关闭内存映射文件
	CloseHandle(hFileMapping);

	cout << "Num of Char A: " << qwNumOfCharA << endl;
}

//不同进程之间共享数据
void ShareData()
{
	HANDLE hPageMap = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 64 * 1024, L"GuiQuan-Map-Page");
	if (hPageMap != NULL)
	{
		if (GetLastError() == ERROR_ALREADY_EXISTS)
		{
			//消费者：读取数据
			PBYTE pbData = (PBYTE)MapViewOfFile(hPageMap, FILE_MAP_READ, 0, 0, 0);

			cout << "Read: " << (char *)pbData << endl;

			//关闭映射
			UnmapViewOfFile(pbData);
		}
		else if (GetLastError() == NO_ERROR)
		{
			//生产者：写入数据
			PBYTE pbData = (PBYTE)MapViewOfFile(hPageMap, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);

			memcpy(pbData, "Hello\0", 6);

			cout << "Write: " << pbData << endl;

			//关闭之后，消费者进程将收不到数据
			getchar();

			//关闭映射
			UnmapViewOfFile(pbData);
		}

		CloseHandle(hPageMap);
	}
}

//创建稀疏文件
void CreateSparseFile()
{
	HANDLE hFileMapping = CreateFileMapping(
		INVALID_HANDLE_VALUE, 
		NULL, 
		PAGE_READWRITE | SEC_RESERVE, 
		0, 
		100 * sizeof(CELL_DATA), 
		L"GuiQuan-Spa-File");

	//映射到进程，预订一块虚拟地址空间
	PBYTE pbData = (PBYTE)MapViewOfFile(hFileMapping, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);

	//调拨物理存储器
	VirtualAlloc(pbData, 100 * sizeof(CELL_DATA), MEM_COMMIT, PAGE_READWRITE);

	//访问数据，保存数据
	pbData[0] = 'A';

	//关闭映射
	UnmapViewOfFile(pbData);

	CloseHandle(hFileMapping);
}