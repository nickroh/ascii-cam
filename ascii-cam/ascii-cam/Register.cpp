#include <windows.h>
#include <shlwapi.h>
#include "CLSID.h"
#include <olectl.h>

#pragma comment(lib, "Shlwapi.lib")

STDAPI DllRegisterServer()
{
    wchar_t clsidStr[64];
    StringFromGUID2(CLSID_MyMediaSource, clsidStr, 64);

    wchar_t keyPath[256];
    wsprintf(keyPath, L"CLSID\\%s\\InprocServer32", clsidStr);

    HKEY hKey;
    LONG res = RegCreateKeyEx(
        HKEY_CLASSES_ROOT,
        keyPath,
        0,
        nullptr,
        REG_OPTION_NON_VOLATILE,
        KEY_WRITE,
        nullptr,
        &hKey,
        nullptr
    );

    if (res != ERROR_SUCCESS)
        return SELFREG_E_CLASS;

    // 🔥 DLL 경로 얻기 (중요!!)
    wchar_t modulePath[MAX_PATH];
    HMODULE hModule = nullptr;

    GetModuleHandleEx(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
        (LPCWSTR)&DllRegisterServer,
        &hModule
    );

    GetModuleFileName(hModule, modulePath, MAX_PATH);

    // DLL 경로 등록
    RegSetValueEx(
        hKey,
        nullptr,
        0,
        REG_SZ,
        (BYTE*)modulePath,
        (lstrlen(modulePath) + 1) * sizeof(wchar_t)
    );

    // ThreadingModel
    const wchar_t* model = L"Both";
    RegSetValueEx(
        hKey,
        L"ThreadingModel",
        0,
        REG_SZ,
        (BYTE*)model,
        (lstrlen(model) + 1) * sizeof(wchar_t)
    );

    RegCloseKey(hKey);
    return S_OK;
}