// dllmain.cpp : ���� DLL Ӧ�ó������ڵ㡣
#include "stdafx.h"

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		//DLL��һ��ӳ�䵽���̵�ַʱ֪ͨ
		break;
	case DLL_THREAD_ATTACH:
		//���̴������߳�ʱ��DLL��ʼ�������ṩDllMain���������������̴߳��������ٵ�Ч�ʣ�
		break;
	case DLL_THREAD_DETACH:
		//����ֹͣ�߳�ʱ��DLL��������
		break;
	case DLL_PROCESS_DETACH:
		//DLL����ӳ�䵽���̵�ַʱ֪ͨ
		break;
	}
	return TRUE;
}

