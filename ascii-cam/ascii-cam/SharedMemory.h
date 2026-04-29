#pragma once
#include <windows.h>
#include <string>

#pragma pack(push, 1)
struct FrameHeader {
    int width;
    int height;
    int channels;
    size_t dataSize;
    long long timestamp;
};
#pragma pack(pop)

class SharedMemory {
public:
    // 생성자에서 초기화 리스트를 지우고 선언만 남깁니다.
    SharedMemory(const std::string& name, size_t size);
    ~SharedMemory();

    bool WriteFrame(const FrameHeader& header, const void* pixelData);
    bool ReadFrame(FrameHeader& outHeader, void* outPixelData);

    // IsValid는 구현이 포함되어 있으므로 세미콜론 없이 이대로 둡니다.
    bool IsValid() const { return m_pBuffer != nullptr; }

private:
    HANDLE m_hMapFile;
    void* m_pBuffer;
    size_t m_size;
};