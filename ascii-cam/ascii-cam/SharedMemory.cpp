#include "SharedMemory.h"

SharedMemory::SharedMemory(const std::string& name, size_t size)
    : m_size(size), m_hMapFile(NULL), m_pBuffer(NULL) {

    // 1. 공유 메모리 영역 생성 (또는 이미 있으면 오픈)
    m_hMapFile = CreateFileMappingA(
        INVALID_HANDLE_VALUE,    // 실제 파일이 아닌 시스템 페이지 파일 사용
        NULL,                    // 보안 속성
        PAGE_READWRITE,          // 읽기/쓰기 권한
        0,                       // 크기(상위 4바이트)
        static_cast<DWORD>(size),// 크기(하위 4바이트)
        name.c_str()             // 공유 메모리 이름
    );

    if (m_hMapFile == NULL) {
        std::cerr << "Could not create file mapping: " << GetLastError() << std::endl;
        return;
    }

    // 2. 프로세스의 주소 공간에 메모리 연결(Map)
    m_pBuffer = MapViewOfFile(
        m_hMapFile,
        FILE_MAP_ALL_ACCESS,
        0, 0, size
    );

    if (m_pBuffer == NULL) {
        std::cerr << "Could not map view of file: " << GetLastError() << std::endl;
        CloseHandle(m_hMapFile);
    }
}

SharedMemory::~SharedMemory() {
    if (m_pBuffer) UnmapViewOfFile(m_pBuffer);
    if (m_hMapFile) CloseHandle(m_hMapFile);
}

void SharedMemory::Write(const void* data, size_t size) {
    if (m_pBuffer && data) {
        // 실제 데이터 복사 (버퍼 오버플로우 방지를 위해 최소값 사용)
        memcpy(m_pBuffer, data, (size < m_size) ? size : m_size);
    }
}

void SharedMemory::Read(void* data, size_t size) {
    if (m_pBuffer && data) {
        memcpy(data, m_pBuffer, (size < m_size) ? size : m_size);
    }
}