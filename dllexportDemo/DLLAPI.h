#pragma once

/*
显式调用：使用LoadLibrary载入动态链接库、使用GetProcAddress获取某函数地址，使用FreeLibrary。

隐式调用：可以使用#pragma comment(lib, “XX.lib”)的方式，也可以直接将XX.lib加入到工程中。

函数调用约定：
__cdecl：调用者清栈，代码小(VC默认)
__stdcall：被调用者清栈(WINAPI回调需要用到，但函数名会重编，需要使用.def文件)

扩展题目：C#可以使用托管C++封装的C++类型，复用非托管C++的一些代码；

*/

#ifdef DLLEXPORTDEMO_EXPORTS
#define DLLAPI extern "C" __declspec(dllexport)
#else
#define DLLAPI extern "C" __declspec(dllimport)
#endif

//导出变量
DLLAPI int max;

//导出函数
DLLAPI void getMax(int x, int y);

