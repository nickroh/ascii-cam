#include "AsciiEngine.h"

AsciiEngine::AsciiEngine(int charWidth, int charHeight)
    : m_charWidth(charWidth), m_charHeight(charHeight) {
    // 기본 문자 세트 설정 (밀도가 높은 문자 -> 낮은 문자)
    m_charset = "@#W$9876543210?!abc;:+=-,._ ";
    BuildCharCache();
}

void AsciiEngine::SetCharset(const std::string& newCharset) {
    m_charset = newCharset;
    BuildCharCache();
}

void AsciiEngine::BuildCharCache() {
    m_charCache.clear();
    for (char c : m_charset) {
        // 문자를 담을 작은 캔버스 생성
        cv::Mat charImg = cv::Mat::zeros(cv::Size(m_charWidth, m_charHeight), CV_8UC1);
        std::string text(1, c);

        // 문자를 이미지 중앙에 가깝게 배치
        cv::putText(charImg, text, cv::Point(0, m_charHeight - 3),
            cv::FONT_HERSHEY_SIMPLEX, 0.35, cv::Scalar(255), 1, cv::LINE_AA);

        m_charCache.push_back(charImg);
    }
}

cv::Mat AsciiEngine::Process(const cv::Mat& frame) {
    if (frame.empty()) return cv::Mat();

    cv::Mat gray, small;
    cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);

    int cols = frame.cols / m_charWidth;
    int rows = frame.rows / m_charHeight;
    cv::resize(gray, small, cv::Size(cols, rows), 0, 0, cv::INTER_AREA);

    cv::Mat asciiFrame = cv::Mat::zeros(frame.size(), CV_8UC1);

    cv::parallel_for_(cv::Range(0, rows), [&](const cv::Range& range) {
        for (int y = range.start; y < range.end; ++y) {
            uint8_t* rowPtr = small.ptr<uint8_t>(y);
            for (int x = 0; x < cols; ++x) {
                uint8_t brightness = rowPtr[x];
                int idx = brightness * (static_cast<int>(m_charset.length()) - 1) / 255;

                cv::Rect roi(x * m_charWidth, y * m_charHeight, m_charWidth, m_charHeight);
                m_charCache[idx].copyTo(asciiFrame(roi));
            }
        }
    });

    return asciiFrame;
}