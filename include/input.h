#ifndef INPUT_H
#define INPUT_H

#include <opencv2/opencv.hpp>
#include <vector>

class InputCapture {
public:
    InputCapture();
    ~InputCapture();

    // 开始录制，持续录制直到用户按下 'e'
    void startCapture();

    // 播放录制过的图像
    void playback();

private:
    cv::VideoCapture cap;
    std::vector<cv::Mat> frames;
};

#endif

