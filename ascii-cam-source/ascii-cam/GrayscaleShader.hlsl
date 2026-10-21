RWTexture2D<float2> outputUV : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    uint width, height;
    outputUV.GetDimensions(width, height);

    if (id.x >= width || id.y >= height)
        return;

    outputUV[id.xy] = float2(128.0 / 255.0, 128.0 / 255.0);
}