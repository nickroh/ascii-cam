#pragma once
#include <opencv2/opencv.hpp>
#include <vector>
#include <string>

class AsciiEngine {
public:
    /**
     * @param charWidth  문자 하나가 차지할 가로 픽셀 크기
     * @param charHeight 문자 하나가 차지할 세로 픽셀 크기
     */
    AsciiEngine(int charWidth = 8, int charHeight = 12);
    ~AsciiEngine() = default;

    // 원본 프레임을 아스키 비트맵 프레임으로 변환
    cv::Mat Process(const cv::Mat& frame) const;

    // 문자 세트 변경 (어두운 -> 밝은 순서)
    void SetCharset(const std::string& newCharset);

private:
    // 각 문자를 미리 이미지로 그려서 저장 (속도 최적화)
    void BuildCharCache();

    std::string m_charset;
    int m_charWidth;
    int m_charHeight;
    std::vector<cv::Mat> m_charCache;
};