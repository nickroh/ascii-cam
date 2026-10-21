#pragma once

#include <windows.h>
#include <mfidl.h>
#include <vector>

class GrayscaleEngine
{
public:
    HRESULT Initialize(UINT width, UINT height);
    HRESULT Process(IMFSample* sample);
    void Shutdown();

private:
    UINT _width = 0;
    UINT _height = 0;

    // Neutral UV data prepared by the grayscale shader.
    std::vector<BYTE> _neutralUV;
};