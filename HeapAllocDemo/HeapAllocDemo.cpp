// HeapAllocDemo.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"

#include "Person.h"
#include <vector>

#include <iostream>

using namespace std;

/*
�ѵ���;���������С�͵����ݽṹ(С��1M)��ʹ�öѹ����������ÿ���ҳ��߽�ͷ������ȣ���������ύ���ͷ�ҳ������⣻
�����ٶȱȽ�������Ϊ��ѯ�������Ѱ�ҿ��е��ڴ�飻

���ֶѣ����̶�(��ʼ��СΪ1M)��Ҳ���Դ���ר�õĶѣ�

���̶ѣ��ɽ�������ʱ�����������˳�ʱ�ͷţ�

C++�Ķѣ�new��deleteʹ�ñ������Ĳ�����������ʹ��Ĭ�ϵĶѷ����ڴ���ͷ��ڴ棻����Ҳ����ʹ��������Щ������ʹ��ר�ŵĶ��������ڴ棻

new VS malloc��new��delete���������ģ����Ե������͵Ĺ��캯��������������

ΪʲôҪʹ��ר�õĶѣ�
1����������б���
2������Ч�����ڹ������һ�������ݽṹ����������ڴ���Ƭ��
3���ֲ����ʣ������е����ݴ�������ڵ�λ�ã�����ҳ�滻������Ĵ�����
4�������߳�ͬ���Ŀ���
5�������ͷţ������ͷ����������ͷ����е����ݿռ䣻

HeadCreate��HeadAlloc��HeadFree��HeadDestroy, ����HeapLock��HeapUnlock
HeadAlloc��HeadFree��HeapSize�����ڲ�����HeapLock��HeapUnlock��ȷ����ͬʱֻ��һ���߳�ռ�ã�
*/

//�޸�ջ�Ĵ�С�������ռ䣬ջ��С(���õݹ麯����������ú����ľ�ջ�Ŀռ䣬���ջ����쳣)
#pragma comment(linker,"/STACK:102400000,1024000")


int main()
{
	vector<Person *> lstPerson;

	//��ʼ������
	for (int i = 0; i < 10000000; i++)
	{
		Person *newPerson = new Person("GuiQuan", true, 22);
		lstPerson.push_back(newPerson);
	}

	//�������
	for (vector<Person*>::iterator itor = lstPerson.begin(); itor != lstPerson.end(); itor++)
	{
		delete *itor;
		*itor = NULL;
	}

	lstPerson.clear();
	
	return 0;
}

