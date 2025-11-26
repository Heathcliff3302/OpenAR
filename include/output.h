#ifndef OUTPUT_H
#define OUTPUT_H

#include <opencv2/opencv.hpp>
#include <string>
#include <vector>

using namespace cv;
using namespace std;

class output
{
public:
    // 将长文本按固定长度分割
    static vector<string> splitText(const string &text, int maxLen);

    // 在摄像头画面上分段显示文本，每段停留 displayTime 秒
    static void showPagedText(VideoCapture &cap,
                              const string &longText,
                              int maxLen = 30,
                              int displayTime = 5);
};

#endif // OUTPUT_H

