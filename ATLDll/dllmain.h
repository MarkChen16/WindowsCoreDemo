// dllmain.h: ģ�����������

class CATLDllModule : public ATL::CAtlDllModuleT< CATLDllModule >
{
public :
	DECLARE_LIBID(LIBID_ATLDllLib)
	DECLARE_REGISTRY_APPID_RESOURCEID(IDR_ATLDLL, "{79053D0A-5001-4B3D-B688-C1AD845109E4}")
};

extern class CATLDllModule _AtlModule;
