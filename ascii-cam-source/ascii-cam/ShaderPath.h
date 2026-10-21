#pragma once

#include <windows.h>
#include <string>

inline std::wstring GetShaderPath(const wchar_t* name)
{
    HMODULE module = nullptr;

    GetModuleHandleExW(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
        GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCWSTR>(&GetShaderPath),
        &module
    );

    if (!module)
        return {};

    wchar_t path[MAX_PATH]{};

    DWORD length = GetModuleFileNameW(module, path, MAX_PATH);

    if (!length || length >= MAX_PATH)
        return {};

    std::wstring result(path, length);
    auto pos = result.find_last_of(L"\\/");

    if (pos == std::wstring::npos)
        return {};

    return result.substr(0, pos + 1) + name;
}