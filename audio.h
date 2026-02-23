#ifndef AUDIO_H
#define AUDIO_H

#include <alsa/asoundlib.h>
#include <iostream>
#include <vector>
#include <cmath>
#include "whisper.h"
#include <string>

// 語音採集與識別線程：錄製 10 秒後做 Whisper 識別，結果寫入 textPromptOut（可為 nullptr 僅錄音）

class audio
{
public:
	void conversion(std::string* textPromptOut);
};
#endif
