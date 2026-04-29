#include "SharedMemory.h"
#include <iostream>

SharedMemory::SharedMemory(const std::string& name, size_t size)
    : m_size(size), m_hMapFile(nullptr), m_pBuffer(nullptr) {

    // 1. 파일 매핑 오브젝트 생성 (페이징 파일 사용)
    m_hMapFile = CreateFileMappingA(
        INVALID_HANDLE_VALUE,    // 물리적 파일 대신 시스템 페이지 파일 사용
        NULL,                    // 기본 보안 속성
        PAGE_READWRITE,          // 읽기/쓰기 권한
        0,                       // 크기(상위 32비트)
        static_cast<DWORD>(size), // 크기(하위 32비트)
        name.c_str()             // 공유 이름
    );

    if (m_hMapFile == NULL) {
        std::cerr << "Could not create file mapping object: " << GetLastError() << std::endl;
        return;
    }

    // 2. 프로세스의 주소 공간에 매핑
    m_pBuffer = MapViewOfFile(
        m_hMapFile,          // 매핑 오브젝트 핸들
        FILE_MAP_ALL_ACCESS, // 전체 접근 권한
        0,
        0,
        size
    );

    if (m_pBuffer == NULL) {
        std::cerr << "Could not map view of file: " << GetLastError() << std::endl;
        CloseHandle(m_hMapFile);
        m_hMapFile = nullptr;
    }
}

SharedMemory::~SharedMemory() {
    if (m_pBuffer) {
        UnmapViewOfFile(m_pBuffer);
        m_pBuffer = nullptr;
    }
    if (m_hMapFile) {
        CloseHandle(m_hMapFile);
        m_hMapFile = nullptr;
    }
}

bool SharedMemory::WriteFrame(const FrameHeader& header, const void* pixelData) {
    if (!IsValid() || (sizeof(FrameHeader) + header.dataSize > m_size)) {
        return false;
    }

    // 헤더 복사
    memcpy(m_pBuffer, &header, sizeof(FrameHeader));

    // 헤더 바로 뒤 위치에 픽셀 데이터 복사
    unsigned char* pDataOffset = static_cast<unsigned char*>(m_pBuffer) + sizeof(FrameHeader);
    memcpy(pDataOffset, pixelData, header.dataSize);

    return true;
}

bool SharedMemory::ReadFrame(FrameHeader& outHeader, void* outPixelData) {
    if (!IsValid()) {
        return false;
    }

    // 헤더 먼저 읽기
    memcpy(&outHeader, m_pBuffer, sizeof(FrameHeader));

    // 픽셀 데이터 읽기
    unsigned char* pDataOffset = static_cast<unsigned char*>(m_pBuffer) + sizeof(FrameHeader);
    memcpy(outPixelData, pDataOffset, outHeader.dataSize);

    return true;
}