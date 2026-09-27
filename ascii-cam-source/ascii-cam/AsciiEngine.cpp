#include "AsciiEngine.h"

#include <algorithm>

#pragma comment(lib, "gdi32.lib")

// Required for the static constexpr array when compiling before C++17.
constexpr char AsciiEngine::CHARSET[];

void AsciiEngine::Initialize(UINT width, UINT height)
{
    _width = width;
    _height = height;
}

void AsciiEngine::Process(BYTE* yPlane, UINT stride) const
{
    if (yPlane == nullptr || _width == 0 || _height == 0 || stride < _width)
        return;

    BITMAPINFO bitmapInfo = {};
    bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmapInfo.bmiHeader.biWidth = static_cast<LONG>(_width);
    bitmapInfo.bmiHeader.biHeight = -static_cast<LONG>(_height); // top-down
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biBitCount = 32;
    bitmapInfo.bmiHeader.biCompression = BI_RGB;

    void* dibPixels = nullptr;
    HBITMAP bitmap = CreateDIBSection(nullptr, &bitmapInfo, DIB_RGB_COLORS,
        &dibPixels, nullptr, 0);
    HDC dc = bitmap ? CreateCompatibleDC(nullptr) : nullptr;
    if (!bitmap || !dc || !dibPixels)
    {
        if (dc) DeleteDC(dc);
        if (bitmap) DeleteObject(bitmap);
        return;
    }

    HGDIOBJ oldBitmap = SelectObject(dc, bitmap);
    const size_t pixelCount = static_cast<size_t>(_width) * _height;
    DWORD* pixels = static_cast<DWORD*>(dibPixels);
    std::fill_n(pixels, pixelCount, 0);
    // A top-down 32-bit DIB stores each pixel as blue, green, red, alpha.
    // GDI writes directly to this buffer while drawing the selected glyphs.
    for (UINT y = 0; y < _height;)
    {
        const UINT cellHeight = (_height - y < CELL_HEIGHT) ? _height - y : CELL_HEIGHT;
        const UINT yEnd = y + cellHeight;

        for (UINT x = 0; x < _width;)
        {
            const UINT cellWidth = (_width - x < CELL_WIDTH) ? _width - x : CELL_WIDTH;
            const UINT xEnd = x + cellWidth;
            UINT sum = 0;
            UINT count = 0;

            // Read the full block before writing so each output value is based
            // on the original luminance values in that block.
            for (UINT py = y; py < yEnd; ++py)
            {
                const BYTE* row = yPlane + static_cast<size_t>(py) * stride;
                for (UINT px = x; px < xEnd; ++px)
                {
                    sum += row[px];
                    ++count;
                }
            }

            const UINT average = sum / count;
            const UINT levelIndex = average * (LEVEL_COUNT - 1) / 255;
            const BYTE value = LEVELS[levelIndex];
            const UINT charsetLength = static_cast<UINT>(sizeof(CHARSET) - 1);
            // The charset runs from dense glyphs to sparse glyphs.
            const UINT characterIndex =
                (sum / count) * (charsetLength - 1) / 255;
            const char character = CHARSET[characterIndex];

            for (UINT py = y; py < yEnd; ++py)
            {
                BYTE* row = yPlane + static_cast<size_t>(py) * stride;
                for (UINT px = x; px < xEnd; ++px)
                    row[px] = value;
            }
            RECT cell = { static_cast<LONG>(x), static_cast<LONG>(y),
                          static_cast<LONG>(xEnd), static_cast<LONG>(yEnd) };
            FillRect(dc, &cell, static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
            SetTextColor(dc, RGB(255, 255, 255));
            SetBkMode(dc, TRANSPARENT);
            DrawTextA(dc, &character, 1, &cell,
                DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

            x = xEnd;
        }

        y = yEnd;
    }

    // Convert the rendered monochrome glyph image back to the caller's Y plane.
    for (UINT py = 0; py < _height; ++py)
    {
        BYTE* outputRow = yPlane + static_cast<size_t>(py) * stride;
        const DWORD* inputRow = pixels + static_cast<size_t>(py) * _width;
        for (UINT px = 0; px < _width; ++px)
        {
            const DWORD pixel = inputRow[px];
            const BYTE blue = static_cast<BYTE>(pixel & 0xff);
            const BYTE green = static_cast<BYTE>((pixel >> 8) & 0xff);
            const BYTE red = static_cast<BYTE>((pixel >> 16) & 0xff);
            outputRow[px] = static_cast<BYTE>((77 * red + 150 * green + 29 * blue) >> 8);
        }
    }

    SelectObject(dc, oldBitmap);
    DeleteDC(dc);
    DeleteObject(bitmap);
}