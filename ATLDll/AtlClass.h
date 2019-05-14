// AtlClass.h : CAtlClass ������

#pragma once
#include "resource.h"       // ������



#include "ATLDll_i.h"



#if defined(_WIN32_WCE) && !defined(_CE_DCOM) && !defined(_CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA)
#error "Windows CE ƽ̨(�粻�ṩ��ȫ DCOM ֧�ֵ� Windows Mobile ƽ̨)���޷���ȷ֧�ֵ��߳� COM ���󡣶��� _CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA ��ǿ�� ATL ֧�ִ������߳� COM ����ʵ�ֲ�����ʹ���䵥�߳� COM ����ʵ�֡�rgs �ļ��е��߳�ģ���ѱ�����Ϊ��Free����ԭ���Ǹ�ģ���Ƿ� DCOM Windows CE ƽ̨֧�ֵ�Ψһ�߳�ģ�͡�"
#endif

using namespace ATL;


// CAtlClass

class ATL_NO_VTABLE CAtlClass :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CAtlClass, &CLSID_AtlClass>,
	public IDispatchImpl<IAtlClass, &IID_IAtlClass, &LIBID_ATLDllLib, /*wMajor =*/ 1, /*wMinor =*/ 0>
{
public:
	CAtlClass()
	{
		m_lgResult = 0;
	}

DECLARE_REGISTRY_RESOURCEID(IDR_ATLCLASS)


BEGIN_COM_MAP(CAtlClass)
	COM_INTERFACE_ENTRY(IAtlClass)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()



	DECLARE_PROTECT_FINAL_CONSTRUCT()

	HRESULT FinalConstruct()
	{
		return S_OK;
	}

	void FinalRelease()
	{
	}

private:
	LONG m_lgResult;

public:



	STDMETHOD(init)(LONG lgType);
	STDMETHOD(add)(LONG lgX, LONG lgY);
	STDMETHOD(get_result)(LONG* pVal);
};

OBJECT_ENTRY_AUTO(__uuidof(AtlClass), CAtlClass)
