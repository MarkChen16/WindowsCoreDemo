// dllmain.cpp: DllMain ��ʵ�֡�

#include "stdafx.h"
#include "resource.h"
#include "ATLDll_i.h"
#include "dllmain.h"

CATLDllModule _AtlModule;

// DLL ��ڵ�
extern "C" BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	hInstance;
	return _AtlModule.DllMain(dwReason, lpReserved); 
}
