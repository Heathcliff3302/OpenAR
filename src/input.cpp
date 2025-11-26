#include "input.h"
#include <iostream>
#include <chrono>
#include <thread>

using namespace std;
using namespace cv;

InputCapture::InputCapture() {
    cap.open(0); // 摄像头0
    if (!cap.isOpened()) {
        cerr << " 摄像头打开失败！" << endl;
    }
}

InputCapture::~InputCapture() {
    cap.release();
}

void InputCapture::startCapture() {
    if (!cap.isOpened()) return;

    cout << " 开始录制，按 e 结束录制" << endl;

    frames.clear();

    auto startTime = chrono::steady_clock::now();
    auto lastCapture = startTime;

    while (true) {
        Mat frame;
        cap >> frame;

        if (frame.empty()) continue;

        // 当前时间
        auto now = chrono::steady_clock::now();

        // 每隔 0.5s 存一张
        if (chrono::duration_cast<chrono::milliseconds>(now - lastCapture).count() >= 500) {
            frames.push_back(frame.clone());
            cout << " 采集到一帧，当前帧数: " << frames.size() << endl;
            lastCapture = now;
        }

        // 实时显示正在录制画面
        imshow("Recording...", frame);

        // 退出录制：按 e
        char c = (char)waitKey(1);
        if (c == 'e' || c == 'E') {
            cout << " 录制结束" << endl;
            destroyWindow("Recording...");
            break;
        }

        // 自动录满 5 秒也结束（可选）
        if (chrono::duration_cast<chrono::seconds>(now - startTime).count() >= 5) {
            cout << " 已录制 5 秒，停止" << endl;
            destroyWindow("Recording...");
            break;
        }
    }
}

void InputCapture::playback() {
    if (frames.empty()) {
        cout << " 没有录到数据" << endl;
        return;
    }

    cout << " 开始回放，共 " << frames.size() << " 帧" << endl;

    for (size_t i = 0; i < frames.size(); i++) {
        imshow("Playback", frames[i]);
        waitKey(500);  // 每张显示 0.5s
    }

    destroyWindow("Playback");
}

