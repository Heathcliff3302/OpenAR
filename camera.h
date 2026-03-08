#ifndef CAMERA_H
#define CAMERA_H

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

#ifndef JPEG_STD_INTERFACE_OPTIONAL
#define JPEG_STD_INTERFACE_OPTIONAL
#endif
#include <jpeglib.h>

#define WIDTH 640
#define HEIGHT 480
#define BUFFER_COUNT 4
#define CAPTURE_INTERVAL_MS 500  // 每500ms采集一帧


int v4l2_init(const char *device);
std::vector<std::string> v4l2_work();  // 返回base64字符串的vector
int v4l2_free();

//辅助函数
long long now_ms();  // 获取当前时间戳（毫秒）
int create_output_dir();  // 创建输出目录

#endif // V4L2_H

