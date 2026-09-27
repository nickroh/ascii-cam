#pragma once

#include <windows.h>
#include <mfidl.h>
#include <cstdint>
#include "AsciiEngine.h"

enum class FilterOption : uint8_t
{
    Passthrough = 0,
    ASCII = 1,
    Edge = 2,
    Grayscale = 3
};

class Filter
{
public:
    Filter();
    ~Filter();
    HRESULT Initialize(UINT width, UINT height, FilterOption option);
    HRESULT Process(IMFSample* sample);
    void Shutdown();

private:
    UINT _width = 0;
    UINT _height = 0;
    FilterOption _option = FilterOption::Passthrough;

    AsciiEngine _ascii;
};