#pragma once

/*
��ʽ���ã�ʹ��LoadLibrary���붯̬���ӿ⡢ʹ��GetProcAddress��ȡĳ������ַ��ʹ��FreeLibrary��

��ʽ���ã�����ʹ��#pragma comment(lib, ��XX.lib��)�ķ�ʽ��Ҳ����ֱ�ӽ�XX.lib���뵽�����С�

��������Լ����
__cdecl����������ջ������С(VCĬ��)
__stdcall������������ջ(WINAPI�ص���Ҫ�õ��������������ر࣬��Ҫʹ��.def�ļ�)

��չ��Ŀ��C#����ʹ���й�C++��װ��C++���ͣ����÷��й�C++��һЩ���룻

*/

#ifdef DLLEXPORTDEMO_EXPORTS
#define DLLAPI extern "C" __declspec(dllexport)
#else
#define DLLAPI extern "C" __declspec(dllimport)
#endif

//��������
DLLAPI int max;

//��������
DLLAPI void getMax(int x, int y);

