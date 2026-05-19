#ifndef PCH_H
#define PCH_H
#ifndef RETURN_IF_FAILED
#define RETURN_IF_FAILED(hr) \
    do { \
        HRESULT __hr = (hr); \
        if (FAILED(__hr)) { return __hr; } \
    } while (0)
#endif
#ifndef RETURN_HR_IF_NULL
#define RETURN_HR_IF_NULL(hr, ptr) \
    do { \
        if ((ptr) == nullptr) { return (hr); } \
    } while (0)
#endif
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "Mfreadwrite.lib")
#pragma comment(lib, "mf.lib")
#pragma comment(lib, "Propsys.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "d2d1.lib")
#endif //PCH_H
