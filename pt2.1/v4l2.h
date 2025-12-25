#ifndef V4L2_H
#define V4L2_H

#include <linux/videodev2.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <vector>
#include <string>
#include <chrono>
#include <memory>

// JPEG庫需要在使用前定義這些宏
#ifndef JPEG_STD_INTERFACE_OPTIONAL
#define JPEG_STD_INTERFACE_OPTIONAL
#endif
#include <jpeglib.h>

// 常量定義
#define WIDTH 640
#define HEIGHT 480
#define BUFFER_COUNT 4
#define CAPTURE_INTERVAL_MS 500  // 每500ms采集一幀

// 函數聲明
int v4l2_init(const char *device);
std::vector<std::string> v4l2_work();  // 返回base64字符串的vector
int v4l2_free();

// 輔助函數聲明（如果需要外部調用）
long long now_ms();  // 獲取當前時間戳（毫秒）
int create_output_dir();  // 創建輸出目錄

#endif // V4L2_H

