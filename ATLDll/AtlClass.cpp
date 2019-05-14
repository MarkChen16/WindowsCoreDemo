// AtlClass.cpp : CAtlClass 的实现

#include "stdafx.h"
#include "AtlClass.h"


// CAtlClass



STDMETHODIMP CAtlClass::init(LONG lgType)
{
	// TODO: 在此添加实现代码
	m_lgResult = 0;

	return S_OK;
}


STDMETHODIMP CAtlClass::add(LONG lgX, LONG lgY)
{
	// TODO: 在此添加实现代码
	m_lgResult = lgX + lgY;

	return S_OK;
}


STDMETHODIMP CAtlClass::get_result(LONG* pVal)
{
	// TODO: 在此添加实现代码
	*pVal = m_lgResult;

	return S_OK;
}
