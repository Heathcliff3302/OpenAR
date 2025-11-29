#ifndef INPUT_H
#define INPUT_H

#include <opencv2/opencv.hpp>
#include <vector>

class InputCapture {
public:
    void init();
    ~InputCapture();

    // 开始录制，持续录制直到用户按下 'e'
    void startCapture();

    // 播放录制过的图像
    void playback();

    // 获取录制到的所有帧
    std::vector<cv::Mat>& getFrames();

    // 访问底层摄像头，便于在其他模块中直接使用
    cv::VideoCapture& getCapture();

private:
    cv::VideoCapture cap;
    std::vector<cv::Mat> frames;
};

#endif

