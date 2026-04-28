#pragma once
#include <windows.h>
#include <string>
#include <iostream>

struct FrameHeader {
    int width;
    int height;
    int channels;
    size_t dataSize;
    long long timestamp; // 프레임 동기화용
};

class SharedMemory {
public:
    // name: 메모리 식별 이름, size: 할당할 바이트 크기
    SharedMemory(const std::string& name, size_t size);
    ~SharedMemory();

    // 데이터 쓰기 (엔진 쪽에서 사용)
    void Write(const void* data, size_t size);

    // 데이터 읽기 (가상 카메라 쪽에서 사용)
    void Read(void* data, size_t size);

    bool IsValid() const { return m_pBuffer != nullptr; }

private:
    HANDLE m_hMapFile;  // file mapping  handle
    void* m_pBuffer;    // shared mem start addr
    size_t m_size;      // total size
};