#pragma once

#include <Windows.h>
#include <string>

//使用专门的堆存放这个类型的所有实例
class Person
{
public:
	Person(const char *szName, bool bSex, int age);
	~Person();

	const char* getName();
	bool getSex();
	int getAge();

	//重载new和delete函数，使用专门的堆
	PVOID operator new(size_t size);
	void operator delete(PVOID p);

private:
	static HANDLE smHeap;
	static UINT smNumAlloc;

	char mName[100];
	bool mSex;
	int mAge;
};

