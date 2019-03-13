// VirtualMemoryDemo.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"

#include <Windows.h>

#include <iostream>

#include <string.h>

using namespace std;

/*
�����ڴ����;��������͵����ݺ����ݽṹ(����1M)
�ѵ���;���������С�͵����ݽṹ(С��1M)��Ĭ��ʹ�öѹ����������ÿ���ҳ��߽�ͷ������ȣ������ύ���ͷ�ҳ������⣻�����ٶȱȽ�����
ջ����;�����ڴ�ź��������;ֲ��������ɱ������Զ�����

����洢�����ڴ桢ҳ�����ļ���Ӳ���ļ���

�ڴ��ҳ���ƣ�

���������ڴ��Ѱַ��ʽ�������ַ�ռ��ҳ��+ƫ���� -�� MMU��ͨ��ӳ����ȡ�����ڴ��ҳ�ţ� -�� �����ڴ��ҳ��+ƫ����

�����ڴ�ԭ������Ҳ���ӳ�������ҳ�ţ�����ȱҳ�жϺ������ֹ���У�ֱ��ִ�������ڴ��ҳ�����ļ��Ļ������������

���̣�ϵͳ�г���ִ�е����ݺ���Դ���ϵĻ�����λ��x86 32λ����Ѱַ�������4G��x64 64����Ѱַ�������16T
�ڴ�ģ�Ͱ�������ָ�������û�ģʽ������64K����������ں�ģʽ����(����ں˶���)

ϵͳ�������̣�����һ���µĽ����ں˶��󡢴���һ��˽�е�ַ�ռ䣻

�û�ģʽ������
	ȫ�ֱ����;�̬����
	ջ�����������;ֲ�������1M���·��䣩
	�ѣ�new��malloc��x86 2G�������ַ�ռ����Ϸ��䣩
	���ֳ���
	�������

�̣߳�ϵͳ����ִ�е�����͵��ȵ���С��λ
ÿ���̶߳��ж�����ջ���Ĵ�����ϵͳ���߳��������л�ʱ��Ҫ��ԭ�ֳ����ڷ����ʱ��Ƭִ�����޵�ָ�

VirtualAlloc
VirtualProtect
VirtualFree
*/


//intRow X intCol�ı��
int intRow = 256, intCol = 1024;

//��Ԫ�����ݣ�ҳ��С4K�ı���
struct CELL_DATA
{
	//����ҳ�Ĵ�С4K
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
	if (pAddr == NULL) cout << "����ռ�ʧ��!" << endl;
	else printf("ָ���ַ=%x\n", pAddr);
	cout << "�ڴ深æ�̶�=" << memStatusVirtualNew.dwMemoryLoad << endl;
	cout << "�������ڴ�=" << memStatusVirtualNew.dwTotalPhys << endl;
	cout << "���������ڴ�=" << memStatusVirtualNew.dwAvailPhys << endl;
	cout << "��ҳ�ļ�=" << memStatusVirtualNew.dwTotalPageFile << endl;
	cout << "����ҳ�ļ�=" << memStatusVirtualNew.dwAvailPageFile << endl;
	cout << "�ܽ��̿ռ�=" << memStatusVirtualNew.dwTotalVirtual << endl;
	cout << "���ý��̿ռ�=" << memStatusVirtualNew.dwAvailVirtual << endl;

	cout << "���������ڴ�仯=" << (int)memStatusVirtualOld.dwAvailPhys - memStatusVirtualNew.dwAvailPhys << endl;
	cout << "����ҳ�ļ��仯=" << (int)memStatusVirtualOld.dwAvailPageFile - memStatusVirtualNew.dwAvailPageFile << endl;
	cout << "���������ַ�ռ�仯="	<< (int)memStatusVirtualOld.dwAvailVirtual - memStatusVirtualNew.dwAvailVirtual << endl;
	cout << endl;

	memStatusVirtualOld = memStatusVirtualNew;
}

//�ύ�������ڴ�
bool SetCellData(LPVOID pAddr, int x, int y, const char *szData)
{
	int intIndex = x * intRow + y;
	if (intIndex > intRow * intCol - 1) return false;

	CELL_DATA *pCurrCell = (CELL_DATA *)pAddr + intIndex;

	//��ѯ�ڴ�����Ϣ
	MEMORY_BASIC_INFORMATION info;
	VirtualQuery(pCurrCell, &info, sizeof(MEMORY_BASIC_INFORMATION));

	//�����û�ύ
	if (info.State != MEM_COMMIT)
	{
		LPVOID lpCurr = VirtualAlloc(pCurrCell, sizeof(CELL_DATA), MEM_COMMIT, PAGE_READWRITE);
		ShowMemoryStatus("�ύ�������ڴ棺", pCurrCell);
	}
	
	if (info.Protect == PAGE_READWRITE || info.Protect == 0)
	{
		strcpy(pCurrCell->szData, szData);
		pCurrCell->IsUsed = true;
	}
	else
	{
		printf("%d, %d��Ԫ���Ѿ�����Ϊ������\n\n", x, y);
		return false;
	}

	return true;
}

//�������
bool ClearCellData(LPVOID pAddr, int x, int y)
{
	int intIndex = x * intRow + y;
	if (intIndex > intRow * intCol - 1) return false;

	CELL_DATA *pCurrCell = (CELL_DATA *)pAddr + intIndex;

	VirtualAlloc((CELL_DATA *)pAddr + intIndex, sizeof(CELL_DATA), MEM_RESET, PAGE_READWRITE);	//���ҳ����ܲ����������

	return true;
}

//�ͷ������ڴ�
bool DeleteCellData(LPVOID pAddr, int x, int y)
{
	int intIndex = x * intRow + y;
	if (intIndex > intRow * intCol - 1) return false;

	CELL_DATA *pCurrCell = (CELL_DATA *)pAddr + intIndex;

	//��ѯ�ڴ�����Ϣ
	MEMORY_BASIC_INFORMATION info;
	VirtualQuery(pCurrCell, &info, sizeof(MEMORY_BASIC_INFORMATION));

	//����ύ�������ڴ棬���ͷ������ڴ�
	if (info.State == MEM_COMMIT)
	{
		VirtualFree((CELL_DATA *)pAddr + intIndex, sizeof(CELL_DATA), MEM_DECOMMIT);
	}

	return true;
}

//���±�������
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

	cout << "�����ڴ�������ԣ�" << endl;
	cout << "ҳ��С=" << sysInfo.dwPageSize << endl;
	cout << "��������=" << sysInfo.dwAllocationGranularity << endl;
	cout << "�û�����Сֵ=" << sysInfo.lpMinimumApplicationAddress << endl;
	cout << "�û������ֵ="	<< sysInfo.lpMaximumApplicationAddress << endl << endl;

	//��ȡ֮ǰ�ڴ����
	GlobalMemoryStatus(&memStatusVirtualOld);

	//MEM_RESERVE:�������������ַ�ռ䡢MEM_TOP_DOWN���Ӷ�����ʼ���䣬����������Ƭ
	intRow = 300;
	intCol = 1024;
	SIZE_T nSize = intRow * intCol * sizeof(CELL_DATA);	//���1G�ռ䣬��������64K�ı���

	LPVOID pVirtual = VirtualAlloc(NULL, nSize, MEM_RESERVE, PAGE_READWRITE);
	if (pVirtual == NULL)
	{
		cout << "һ���ӱ���������ô�������ַ�ռ䣡" << endl;
		return 0;
	}

	ShowMemoryStatus("�������������ַ�ռ䣺", pVirtual);

	//ȫ���ύ�������ڴ�
	LPVOID pPhysics = NULL;
	pPhysics = VirtualAlloc(pVirtual, nSize, MEM_COMMIT, PAGE_READWRITE);
	if (pPhysics == NULL)
	{
		cout << "һ���ӷ��䲻����ô�������ڴ棡" << endl;
	}
	else
	{
		//ȫ����ʼ��
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

	//���õ�Ԫ�����ݣ���Ҫʹ�õĵ�Ԫ����ύ�������ڴ棬�޸����ݲŻ����ʵ�ʵ��ڴ�ռ�
	SetCellData(pVirtual, 0, 50, "123");
	ClearCellData(pVirtual, 0, 50);
	DeleteCellData(pVirtual, 0, 50);

	SetCellData(pVirtual, 0, 50, "456");
	SetCellData(pVirtual, 0, 51, "789");

	//���ĳ����������
	SetCellData(pVirtual, 1, 30, "AAA");
	ClearCellData(pVirtual, 1, 30);
	SetCellData(pVirtual, 1, 30, "BBB");

	//���ñ�������
	ProtectCellData(pVirtual, 1, 30, true);
	SetCellData(pVirtual, 1, 30, "CCC");
	ProtectCellData(pVirtual, 1, 30, false);
	SetCellData(pVirtual, 1, 30, "DDD");

	//�ͷ�ĳ����Ԫ�������ռ�
	DeleteCellData(pVirtual, 1, 30);

	//MEM_RELEASE���ͷ����������������ַ�ռ�������ڴ�(dSize����Ҫ����Ϊ0)
	VirtualFree(pVirtual, 0, MEM_RELEASE);
	ShowMemoryStatus("�ͷ����������������ַ�ռ�������ڴ棺", pVirtual);

	int i;
	cin >> i;

    return 0;
}

