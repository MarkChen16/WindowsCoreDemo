#pragma once

#include <Windows.h>
#include <string>

//ʹ��ר�ŵĶѴ��������͵�����ʵ��
class Person
{
public:
	Person(const char *szName, bool bSex, int age);
	~Person();

	const char* getName();
	bool getSex();
	int getAge();

	//����new��delete������ʹ��ר�ŵĶ�
	PVOID operator new(size_t size);
	void operator delete(PVOID p);

private:
	static HANDLE smHeap;
	static UINT smNumAlloc;

	char mName[100];
	bool mSex;
	int mAge;
};

