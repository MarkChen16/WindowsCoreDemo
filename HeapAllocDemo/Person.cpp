#include "stdafx.h"
#include "Person.h"

HANDLE Person::smHeap = NULL;
UINT Person::smNumAlloc = 0;

Person::Person(const char *szName, bool bSex, int age)
{
	strcpy(mName, szName);
	mSex = bSex;
	mAge = age;
}

Person::~Person()
{
}

const char * Person::getName()
{
	return nullptr;
}

bool Person::getSex()
{
	return false;
}

int Person::getAge()
{
	return 0;
}

PVOID Person::operator new(size_t size)
{
	//创建专门的堆
	if (NULL == smHeap)
	{
		smHeap = HeapCreate(HEAP_GENERATE_EXCEPTIONS, 0, 0);

		if (NULL == smHeap) return NULL;
	}

	//在堆上分配内存
	PVOID p = HeapAlloc(smHeap, 0, size);
	if (NULL != p) smNumAlloc++;

	return p;
}

void Person::operator delete(PVOID p)
{
	//如果堆已经销毁
	if (NULL == smHeap) return;

	//在堆上释放内存
	if (HeapFree(smHeap, 0, p))
	{
		smNumAlloc--;
	}

	//销毁专门的堆
	if (smNumAlloc == 0)
	{
		if (HeapDestroy(smHeap))
		{
			smHeap = NULL;
		}
	}
}
