// VirtualMemoryDemo.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"

#include <Windows.h>

#include <iostream>

#include <string.h>

using namespace std;

//intRow X intCol的表格
int intRow = 256, intCol = 1024;

//单元格数据，页大小4K的倍数
struct CELL_DATA
{
	//对齐页的大小4K
	union
	{
		char szData[1024];
		char szReserve[4096-1];
	};

	bool IsUsed;
};

MEMORYSTATUS memStatusVirtualOld;
void ShowMemoryStatus(const char *szTitle, LPVOID pAddr)
{
	MEMORYSTATUS memStatusVirtualNew;
	GlobalMemoryStatus(&memStatusVirtualNew);
	cout << szTitle << endl;
	if (pAddr == NULL) cout << "分配空间失败!" << endl;
	else printf("指针地址=%x\n", pAddr);
	cout << "内存繁忙程度=" << memStatusVirtualNew.dwMemoryLoad << endl;
	cout << "总物理内存=" << memStatusVirtualNew.dwTotalPhys << endl;
	cout << "可用物理内存=" << memStatusVirtualNew.dwAvailPhys << endl;
	cout << "总页文件=" << memStatusVirtualNew.dwTotalPageFile << endl;
	cout << "可用页文件=" << memStatusVirtualNew.dwAvailPageFile << endl;
	cout << "总进程空间=" << memStatusVirtualNew.dwTotalVirtual << endl;
	cout << "可用进程空间=" << memStatusVirtualNew.dwAvailVirtual << endl;

	cout << "可用物理内存变化=" << (int)memStatusVirtualOld.dwAvailPhys - memStatusVirtualNew.dwAvailPhys << endl;
	cout << "可用页文件变化=" << (int)memStatusVirtualOld.dwAvailPageFile - memStatusVirtualNew.dwAvailPageFile << endl;
	cout << "可用虚拟地址空间变化="	<< (int)memStatusVirtualOld.dwAvailVirtual - memStatusVirtualNew.dwAvailVirtual << endl;
	cout << endl;

	memStatusVirtualOld = memStatusVirtualNew;
}

//提交到物理内存
bool SetCellData(LPVOID pAddr, int x, int y, const char *szData)
{
	int intIndex = x * intRow + y;
	if (intIndex > intRow * intCol - 1) return false;

	CELL_DATA *pCurrCell = (CELL_DATA *)pAddr + intIndex;

	//查询内存块的信息
	MEMORY_BASIC_INFORMATION info;
	VirtualQuery(pCurrCell, &info, sizeof(MEMORY_BASIC_INFORMATION));

	//如果还没提交
	if (info.State != MEM_COMMIT)
	{
		VirtualAlloc(pCurrCell, sizeof(CELL_DATA), MEM_COMMIT, PAGE_READWRITE);
		ShowMemoryStatus("提交到物理内存：", pCurrCell);
	}
	
	if (info.Protect == PAGE_READWRITE || info.Protect == 0)
	{
		strcpy(pCurrCell->szData, szData);
		pCurrCell->IsUsed = true;
	}
	else
	{
		printf("%d, %d单元格已经设置为保护！\n\n", x, y);
		return false;
	}

	return true;
}

//清除数据
bool ClearCellData(LPVOID pAddr, int x, int y)
{
	int intIndex = x * intRow + y;
	if (intIndex > intRow * intCol - 1) return false;

	CELL_DATA *pCurrCell = (CELL_DATA *)pAddr + intIndex;

	VirtualAlloc((CELL_DATA *)pAddr + intIndex, sizeof(CELL_DATA), MEM_RESET, PAGE_READWRITE);	//这个页面可能不会立即清除

	return true;
}

//释放物理内存
bool DeleteCellData(LPVOID pAddr, int x, int y)
{
	int intIndex = x * intRow + y;
	if (intIndex > intRow * intCol - 1) return false;

	CELL_DATA *pCurrCell = (CELL_DATA *)pAddr + intIndex;

	//查询内存块的信息
	MEMORY_BASIC_INFORMATION info;
	VirtualQuery(pCurrCell, &info, sizeof(MEMORY_BASIC_INFORMATION));

	//如果提交到物理内存，则释放物理内存
	if (info.State == MEM_COMMIT)
	{
		VirtualFree((CELL_DATA *)pAddr + intIndex, sizeof(CELL_DATA), MEM_DECOMMIT);
	}

	return true;
}

//更新保护属性
bool ProtectCellData(LPVOID pAddr, int x, int y, bool isLock)
{
	int intIndex = x * intRow + y;
	if (intIndex > intRow * intCol - 1) return false;

	CELL_DATA *pCurrCell = (CELL_DATA *)pAddr + intIndex;

	DWORD dOldProtected = 0;
	if (isLock)
	{
		VirtualProtect((CELL_DATA *)pAddr + intIndex, sizeof(CELL_DATA), PAGE_READONLY, &dOldProtected);
	}
	else
	{
		VirtualProtect((CELL_DATA *)pAddr + intIndex, sizeof(CELL_DATA), PAGE_READWRITE, &dOldProtected);
	}

	return true;
}

int main()
{
	SYSTEM_INFO sysInfo;
	GetSystemInfo(&sysInfo);

	cout << "主机内存管理属性：" << endl;
	cout << "页大小=" << sysInfo.dwPageSize << endl;
	cout << "分配粒度=" << sysInfo.dwAllocationGranularity << endl;
	cout << "用户区最小值=" << sysInfo.lpMinimumApplicationAddress << endl;
	cout << "用户区最大值="	<< sysInfo.lpMaximumApplicationAddress << endl << endl;

	//获取之前内存情况
	GlobalMemoryStatus(&memStatusVirtualOld);

	//MEM_RESERVE:保留进程虚拟地址空间、MEM_TOP_DOWN：从顶部开始分配，减少内在碎片
	intRow = 300;
	intCol = 1024;
	SIZE_T nSize = intRow * intCol * sizeof(CELL_DATA);	//大概1G空间，分配粒度64K的倍数

	LPVOID pVirtual = VirtualAlloc(NULL, nSize, MEM_RESERVE, PAGE_READWRITE);
	if (pVirtual == NULL)
	{
		cout << "一下子保留不了这么多虚拟地址空间！" << endl;
		return 0;
	}

	ShowMemoryStatus("保留进程虚拟地址空间：", pVirtual);

	//全部提交到物理内存
	LPVOID pPhysics = VirtualAlloc(pVirtual, nSize, MEM_COMMIT, PAGE_READWRITE);
	if (pPhysics == NULL)
	{
		cout << "一下子分配不了这么多物理内存！" << endl;
	}
	else
	{
		//全部初始化
		for (int i = 0; i < intRow; i++)
		{
			for (int j = 0; j < intCol; j++)
			{
				CELL_DATA *pCurrCell = (CELL_DATA *)pPhysics + i * intRow + j;
				strcpy(pCurrCell->szData, "XXXXXXXXX");
				pCurrCell->IsUsed = true;
			}
		}
	}

	//设置单元格数据，需要使用的单元格才提交到物理内存，修改数据才会分配实际的内存空间
	SetCellData(pVirtual, 0, 50, "123");
	ClearCellData(pVirtual, 0, 50);
	DeleteCellData(pVirtual, 0, 50);

	SetCellData(pVirtual, 0, 50, "456");
	SetCellData(pVirtual, 0, 51, "789");

	//清除某个数据数据
	SetCellData(pVirtual, 1, 30, "AAA");
	ClearCellData(pVirtual, 1, 30);
	SetCellData(pVirtual, 1, 30, "BBB");

	//设置保护属性
	ProtectCellData(pVirtual, 1, 30, true);
	SetCellData(pVirtual, 1, 30, "CCC");
	ProtectCellData(pVirtual, 1, 30, false);
	SetCellData(pVirtual, 1, 30, "DDD");

	//释放某个单元格的物理空间
	DeleteCellData(pVirtual, 1, 30);

	//MEM_RELEASE：释放整个保留的虚拟地址空间和物理内存(dSize参数要设置为0)
	VirtualFree(pVirtual, 0, MEM_RELEASE);
	ShowMemoryStatus("释放整个保留的虚拟地址空间和物理内存：", pVirtual);

	int i;
	cin >> i;

    return 0;
}

