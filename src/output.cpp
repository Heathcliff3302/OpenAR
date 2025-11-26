#include <opencv2/opencv.hpp>
#include <string>
#include <vector>
using namespace cv;
using namespace std;

// 将长文本按固定长度切割
vector<string> output::splitText(const string &text, int maxLen)
{
    vector<string> result;
    for (size_t i = 0; i < text.size(); i += maxLen)
    {
        result.push_back(text.substr(i, maxLen));
    }
    return result;
}

// 在摄像头画面上逐段输出文本，每段停留 displayTime 秒
void output::showPagedText(VideoCapture &cap, const string &longText, int maxLen, int displayTime)
{
    vector<string> segments = splitText(longText, maxLen);

    Mat frame;
    for (const auto &line : segments)
    {
        double start = (double)getTickCount();

        while (true)
        {
            cap >> frame;
            if (frame.empty()) break;

            // 显示文本
            putText(frame,
                    line,
                    Point(50, 100),
                    FONT_HERSHEY_SIMPLEX,
                    1.0,
                    Scalar(0, 255, 0),
                    2);
		namedWindow("CAMERA",WINDOW_NORMAL);
		resizeWindow("CAMERA",640,480);
            imshow("CAMERA", frame);

            // ESC退出
            if (waitKey(10) == 27) return;

            // 显示 displayTime 秒
            double elapsed =
                ((double)getTickCount() - start) / getTickFrequency();
            if (elapsed >= displayTime) break;
        }
    }
}

