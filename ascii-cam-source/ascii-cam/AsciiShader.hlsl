Texture2D<float> inputTex : register(t0);

RWTexture2D<float4> outputTex : register(u0);

static const uint CHAR_WIDTH = 8;
static const uint CHAR_HEIGHT = 16;
static const uint CHAR_COUNT = 10;

static const uint glyphs[CHAR_COUNT][CHAR_HEIGHT] =
{
    {
        0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0
    },

    {
        0,0,0,0,0,0,0,0,
        0x18,0x18,0,0,0,0,0,0
    },

    {
        0,0,0,0,0,0,0,0,
        0x18,0x18,0,0,0,0x18,0x18,0
    },

    {
        0,0,0,0,0,0x7E,0x7E,0,
        0,0,0,0,0,0,0,0
    },

    {
        0,0,0,0,0,0,0x7E,0x7E,
        0,0,0x7E,0x7E,0,0,0,0
    },

    {
        0,0,0,0,0x18,0x18,0x18,0x18,
        0x7E,0x7E,0x18,0x18,0x18,0,0,0
    },

    {
        0,0,0,0,0x18,0x18,0x5A,0x3C,
        0x3C,0x5A,0x18,0x18,0,0,0,0
    },

    {
        0,0,0,0x24,0x24,0x7E,0x7E,0x24,
        0x24,0x7E,0x7E,0x24,0x24,0,0,0
    },

    {
        0,0,0,0x62,0x62,0x64,0x08,0x10,
        0x20,0x4C,0x8C,0x8C,0,0,0,0
    },

    {
        0,0,0,0x3C,0x42,0x99,0xA5,0xA5,
        0x9D,0x40,0x3C,0x3C,0,0,0,0
    }
};

[numthreads(8, 8, 1)]
void main(uint3 dtid : SV_DispatchThreadID)
{
    uint width;
    uint height;

    inputTex.GetDimensions(width, height);

    uint2 pos = dtid.xy;

    if (pos.x >= width || pos.y >= height)
    {
        return;
    }

    uint2 cellStart =
        (pos / uint2(CHAR_WIDTH, CHAR_HEIGHT)) *
        uint2(CHAR_WIDTH, CHAR_HEIGHT);

    uint2 cellEnd = min(
        cellStart + uint2(CHAR_WIDTH, CHAR_HEIGHT),
        uint2(width, height)
    );

    float lumSum = 0.0f;

    uint count = 0;

    for (uint y = cellStart.y; y < cellEnd.y; ++y)
    {
        for (uint x = cellStart.x; x < cellEnd.x; ++x)
        {
            lumSum += inputTex.Load(int3(x, y, 0));

            ++count;
        }
    }

    float luminance =
        lumSum / max((float)count, 1.0f);

    float tone =
        pow(saturate(luminance), 0.82f);

    uint glyphIndex = min(
        (uint)floor(tone * (CHAR_COUNT - 1) + 0.5f),
        CHAR_COUNT - 1
    );

    uint2 local = pos - cellStart;

    uint rowBits = glyphs[glyphIndex][local.y];

    bool ink =
        ((rowBits >> (7 - local.x)) & 1) != 0;

    outputTex[pos] = ink
        ? float4(1.0f, 1.0f, 1.0f, 1.0f)
        : float4(0.0f, 0.0f, 0.0f, 1.0f);
}