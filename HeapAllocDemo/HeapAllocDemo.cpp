// HeapAllocDemo.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"

#include "Person.h"
#include <vector>

#include <iostream>

using namespace std;

/*
堆的用途：管理大量小型的数据结构(小于1M)，使用堆管理器，不用考虑页面边界和分配粒度，还有如何提交和释放页面的问题；
但是速度比较慢，因为查询分配表来寻找空闲的内存块；

各种堆：进程堆(初始大小为1M)，也可以创建专用的堆；

进程堆：由进程启动时创建，进程退出时释放；

C++的堆：new、delete使用编译器的操作符函数，使用默认的堆分配内存和释放内存；我们也可以使用重载这些函数，使用专门的堆来管理内存；

new VS malloc：new和delete是面向对象的，可以调用类型的构造函数和析构函数；

为什么要使用专用的堆：
1、对组件进行保护
2、更有效的内在管理，存放一样的数据结构，避免产生内存碎片；
3、局部访问，将所有的数据存放在相邻的位置，减少页面换出换入的次数；
4、避免线程同步的开销
5、快速释放，可以释放整个堆来释放所有的数据空间；

HeadCreate、HeadAlloc、HeadFree、HeadDestroy, 还有HeapLock和HeapUnlock
HeadAlloc、HeadFree和HeapSize会在内部调用HeapLock和HeapUnlock，确保堆同时只有一个线程占用；
*/

//修改栈的大小，保留空间，栈大小(慎用递归函数，如果调用很深，会耗尽栈的空间，造成栈溢出异常)
#pragma comment(linker,"/STACK:102400000,1024000")


int main()
{
	vector<Person *> lstPerson;

	//初始化向量
	for (int i = 0; i < 10000000; i++)
	{
		Person *newPerson = new Person("GuiQuan", true, 22);
		lstPerson.push_back(newPerson);
	}

	//清除向量
	for (vector<Person*>::iterator itor = lstPerson.begin(); itor != lstPerson.end(); itor++)
	{
		delete *itor;
		*itor = NULL;
	}

	lstPerson.clear();
	
	return 0;
}

