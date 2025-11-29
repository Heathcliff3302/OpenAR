#include <iostream>
#include <opencv2/opencv.hpp>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include "input.h"
#include "output.h"
#include "base64.h"

using namespace std;
using namespace cv;
using json = nlohmann::json;

string extractAIResponse(const string& aiResponseJson) {
	try {
		json j = json::parse(aiResponseJson);
		// SiliconFlow API 返回格式: {"choices": [{"message": {"content": "..."}}]}
		if (j.contains("choices") && j["choices"].is_array() && !j["choices"].empty()) {
			if (j["choices"][0].contains("message")) {
				if (j["choices"][0]["message"].contains("content")) {
					return j["choices"][0]["message"]["content"].get<string>();
				}
			}
		}
		// 如果格式不同，返回原始JSON
		return aiResponseJson;
	}
	catch (const exception& e) {
		return "解析AI响应失败: " + string(e.what()) + "\n原始响应: " + aiResponseJson;
	}
}


int main()
{
    InputCapture ip;
    base64 stai;
    output op;

    string textPrompt = "Please describe the content in the picture";
    string token = "sk-tniblsjfylvkfquhcbzaymwnwmkmgitwhakqycgwwskhctwp";
    string model = "Qwen/Qwen3-VL-32B-Thinking";

    cout << "按下 s 开始录制，按 q 退出" << endl;

    ip.init();

    while (true)
    {
        char c = getchar();
        if (c == '\n') continue;  // 忽略回车

        if (c == 'q' || c == 'Q') {
            cout << "退出程序\n";
            break;
        }

        if (c == 's' || c == 'S')
        {
            cout << "开始录制...\n";
            ip.startCapture();

            const auto& frames = ip.getFrames();
            if (frames.empty()) {
                cout << "未录到任何帧！\n";
                continue;
            }

            // 1. 图像转 base64
            vector<string> base64Imgs;
            for (auto& f : frames)
                base64Imgs.push_back(stai.matToBase64(f));

            // 2. 组 JSON
            string jsonBody = stai.packToJson(base64Imgs, textPrompt);

            // 3. 发给 AI
            string aiResponse = stai.sendToAI(token, model, jsonBody);

            // 4. 显示文字
            string reply = extractAIResponse(aiResponse);
            vector<string> texts = op.splitText(reply, 30);
            
            op.showPagedText(ip.getCapture(), texts, 30, 5);
        }
    }

    return 0;
}




















































