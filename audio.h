#ifndef AUDIO_H
#define AUDIO_H

#include <alsa/asoundlib.h>
#include <iostream>
#include <vector>
#include <cmath>
#include "whisper.h"
#include <string>

// 语音采集线程：录制10秒后做whisper识别，结果写入textPromptOut（可为nullptr仅录音）

class audio
{
public:
	void conversion(std::string* textPromptOut);
};
#endif
