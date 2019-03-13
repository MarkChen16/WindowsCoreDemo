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
	//����ר�ŵĶ�
	if (NULL == smHeap)
	{
		smHeap = HeapCreate(HEAP_GENERATE_EXCEPTIONS, 0, 0);

		if (NULL == smHeap) return NULL;
	}

	//�ڶ��Ϸ����ڴ�
	PVOID p = HeapAlloc(smHeap, 0, size);
	if (NULL != p) smNumAlloc++;

	return p;
}

void Person::operator delete(PVOID p)
{
	//������Ѿ�����
	if (NULL == smHeap) return;

	//�ڶ����ͷ��ڴ�
	if (HeapFree(smHeap, 0, p))
	{
		smNumAlloc--;
	}

	//����ר�ŵĶ�
	if (smNumAlloc == 0)
	{
		if (HeapDestroy(smHeap))
		{
			smHeap = NULL;
		}
	}
}
