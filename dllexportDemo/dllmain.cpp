// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "stdafx.h"

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		//DLL第一次映射到进程地址时通知
		break;
	case DLL_THREAD_ATTACH:
		//进程创建新线程时对DLL初始化（不提供DllMain函数，可以提升线程创建和销毁的效率）
		break;
	case DLL_THREAD_DETACH:
		//进程停止线程时对DLL做清理工作
		break;
	case DLL_PROCESS_DETACH:
		//DLL撤销映射到进程地址时通知
		break;
	}
	return TRUE;
}

