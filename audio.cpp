#include "audio.h"

void audio::conversion(std::string* textPromptOut) {
    const char* device = "hw:3,0";   // 修改成你的设备
    const unsigned int sample_rate = 48000;
    const int channels = 1;
    const int record_seconds = 10;
    const int target_samples = sample_rate * record_seconds;

    snd_pcm_t* handle;
    snd_pcm_hw_params_t* params;

    int rc = snd_pcm_open(&handle, device, SND_PCM_STREAM_CAPTURE, 0);
    if (rc < 0) {
        std::cerr << "Cannot open device\n";
        return;
    }

    snd_pcm_hw_params_alloca(&params);
    snd_pcm_hw_params_any(handle, params);
    snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_channels(handle, params, channels);
    snd_pcm_hw_params_set_rate(handle, params, sample_rate, 0);

    rc = snd_pcm_hw_params(handle, params);
    if (rc < 0) {
        std::cerr << "Cannot set hardware params\n";
        snd_pcm_close(handle);
        return;
    }

    snd_pcm_prepare(handle);

    const int period_size = 1024;
    std::vector<int16_t> buffer(period_size);

    std::vector<int16_t> raw_audio;
    raw_audio.reserve(target_samples);

    std::cout << "Recording 10 seconds..." << std::endl;

    while (raw_audio.size() < target_samples) {
        int frames = snd_pcm_readi(handle, buffer.data(), period_size);

        if (frames < 0) {
            frames = snd_pcm_recover(handle, frames, 0);
            if (frames < 0) {
                std::cerr << "Read error\n";
                break;
            }
        }

        for (int i = 0; i < frames && raw_audio.size() < target_samples; ++i) {
            raw_audio.push_back(buffer[i]);
        }
    }

    snd_pcm_drop(handle);
    snd_pcm_close(handle);

    std::cout << "Raw samples: " << raw_audio.size() << std::endl;

    // 3:1 downsample
    std::vector<int16_t> downsampled;
    downsampled.reserve(raw_audio.size() / 3);

    for (size_t i = 0; i + 2 < raw_audio.size(); i += 3) {
        downsampled.push_back(raw_audio[i]);
    }

    std::cout << "Downsampled samples: " << downsampled.size() << std::endl;

    // convert to float
    std::vector<float> float_audio;
    float_audio.reserve(downsampled.size());

    for (auto s : downsampled) {
        float_audio.push_back(s / 32768.0f);
    }

    if (float_audio.empty()) {
        if (textPromptOut) *textPromptOut = "";
        return;
    }

    // 1. 初始化模型
struct whisper_context_params cparams = whisper_context_default_params();
cparams.use_gpu = false;  // 强制 CPU

struct whisper_context* ctx =
    whisper_init_from_file_with_params(
        "/home/firefly/hzlwkspace/whisper/whisper.cpp-master/models/ggml-base.bin",
        cparams);

if (!ctx) {
    std::cerr << "Failed to load model\n";
    return;
}

// 2. 设置推理参数
struct whisper_full_params wparams =
    whisper_full_default_params(WHISPER_SAMPLING_GREEDY);

wparams.print_progress   = false;
wparams.print_special    = false;
wparams.print_realtime   = false;
wparams.print_timestamps = false;
wparams.translate        = false;
wparams.language         = "zh";   // 如果你说中文

// 3. 执行推理
if (whisper_full(ctx,wparams,
                 float_audio.data(),
                 float_audio.size()) != 0) {
    std::cerr << "Failed to run whisper\n";
    whisper_free(ctx);
    return;
}

// 4. 获取结果
int n_segments = whisper_full_n_segments(ctx);

std::cout << "\nRecognized:\n";

std::string prompt;

for (int i = 0; i < n_segments; ++i) {
    const char* text = whisper_full_get_segment_text(ctx, i);
    prompt += text;
}

// 將識別結果寫入 textPrompt，供主線程合併 JSON 使用
if (textPromptOut) {
    *textPromptOut = prompt;
}

std::cout << std::endl;

// 5. 释放
whisper_free(ctx);
}

