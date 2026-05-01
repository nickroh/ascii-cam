#include "ClassFactory.h"
#include "CLSID.h"

extern "C" HRESULT __stdcall DllGetClassObject(
    REFCLSID rclsid,
    REFIID riid,
    void** ppv)
{
    if (rclsid != CLSID_MyMediaSource)
        return CLASS_E_CLASSNOTAVAILABLE;

    static ClassFactory factory;
    return factory.QueryInterface(riid, ppv);
}