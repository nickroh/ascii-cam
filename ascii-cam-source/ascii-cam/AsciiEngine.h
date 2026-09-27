#pragma once

#include <windows.h>

// Replaces an 8-bit Y plane with a raster image made from ASCII characters.
class AsciiEngine
{
public:
    AsciiEngine() = default;
    ~AsciiEngine() = default;

    void Initialize(UINT width, UINT height);

    // yPlane must point to height rows, each with at least width bytes.
    // stride is the byte distance between rows. Output is white glyphs on black.
    void Process(BYTE* yPlane, UINT stride) const;

private:
    UINT _width = 0;
    UINT _height = 0;

    static constexpr UINT CELL_WIDTH = 8;
    static constexpr UINT CELL_HEIGHT = 12;
    // Kept for compatibility with the earlier block-quantization implementation.
    static constexpr UINT BLOCK_SIZE = 8;
    static constexpr UINT LEVEL_COUNT = 9;
    static constexpr BYTE LEVELS[LEVEL_COUNT] =
    {
        0, 32, 64, 96, 128, 160, 192, 224, 255
    };
    // Ordered from visually dense to sparse, matching the reference behavior.
    static constexpr char CHARSET[] = "@#W$9876543210?!abc;:+=-,._ ";
};