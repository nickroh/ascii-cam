#include <iostream>
#include <opencv2/opencv.hpp>
#include "AsciiEngine.h"

int main() {
    test_ascii();
}

void test_ascii() {
    // 1. Load the image file
    // Replace "test.jpg" with your actual file path. 
    // Supports jpg, png, bmp, etc.
    std::string filePath = "max.jpg";
    cv::Mat image = cv::imread(filePath);

    if (image.empty()) {
        std::cerr << "Error: Could not open or find the image!" << std::endl;
        return -1;
    }

    // 2. Initialize the AsciiEngine
    // Setting character size to 8x12 pixels. 
    // Smaller numbers = Higher detail (more characters)
    AsciiEngine engine(8, 12);

    // 3. Optional: Resize if the image is too large for your monitor
    if (image.cols > 1280) {
        double scale = 1280.0 / image.cols;
        cv::resize(image, image, cv::Size(), scale, scale);
    }

    // 4. Process the image
    cv::Mat asciiImage = engine.Process(image);

    // 5. Display the results
    cv::imshow("Original Image", image);
    cv::imshow("ASCII Art Output", asciiImage);

    // 6. Optional: Save the result to a file
    // cv::imwrite("ascii_result.png", asciiImage);

    std::cout << "Press any key to exit..." << std::endl;
    cv::waitKey(0); // Wait for a keyboard stroke

    return 0;
}