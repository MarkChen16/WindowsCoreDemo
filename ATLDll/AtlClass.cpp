// AtlClass.cpp : CAtlClass ��ʵ��

#include "stdafx.h"
#include "AtlClass.h"


// CAtlClass



STDMETHODIMP CAtlClass::init(LONG lgType)
{
	// TODO: �ڴ����ʵ�ִ���
	m_lgResult = 0;

	return S_OK;
}


STDMETHODIMP CAtlClass::add(LONG lgX, LONG lgY)
{
	// TODO: �ڴ����ʵ�ִ���
	m_lgResult = lgX + lgY;

	return S_OK;
}


STDMETHODIMP CAtlClass::get_result(LONG* pVal)
{
	// TODO: �ڴ����ʵ�ִ���
	*pVal = m_lgResult;

	return S_OK;
}
