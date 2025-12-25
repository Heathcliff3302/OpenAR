无注释版本
可在RK3588上完整运行
整体的功能为：
  1.opencv调用免驱的摄像头(/dev/video0)完成环境的图像数据采集，并以Mat格式在容器中存储起来（input.cpp）
  2.将多张Mat格式的图片转成base64，然后再读取用户提示词构成json接口申请（base64.cpp）
  3.将json发送给云端的qwen多模态AI模型接口，并获取回应文本（base64.cpp）
  4.将json格式的回应分离出string回答，然后切割滚动显示在opencv的摄像头窗口上（output.cpp)
