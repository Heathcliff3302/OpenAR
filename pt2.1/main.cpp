#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include "send2ai.h"
#include "v4l2.h"

using namespace std;
using json = nlohmann::json;

string extractAIResponse(const string& aiResponseJson) {
	try {
		json j = json::parse(aiResponseJson);
		// SiliconFlow API 返回格式: {"choices": [{"message": {"content": "..."}}]}
		if (j.contains("choices") && j["choices"].is_array() && !j["choices"].empty()) 
        //如果j中包含choices，并且choices是一个数组，并且choices不为空
        {
			if (j["choices"][0].contains("message")) {
				if (j["choices"][0]["message"].contains("content")) {
					return j["choices"][0]["message"]["content"].get<string>();
                    //返回choices[0].message.content，即返回的回答内容
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
    base64 stai;

    string textPrompt = "请告诉我图中纸上面的数字";
    string token = "sk-tniblsjfylvkfquhcbzaymwnwmkmgitwhakqycgwwskhctwp";
    string model = "Qwen/Qwen3-VL-32B-Thinking";

    cout << "按下 s 开始录制，按 q 退出" << endl;

    // 初始化v4l2用於采集
    if(v4l2_init("/dev/video0") < 0)
    {
        cout<<"摄像头初始化失败"<<endl;
        return -1;
    }

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

            /*opencv方案
            ip.startCapture();
            开始录制，用时间戳差值存储帧，并在10s后自动退出

           const auto& frames = ip.getFrames();
            const获取容器的只读引用，保护frams中的数据只能在input.cpp中被修改
            if (frames.empty()) {
               cout << "未录到任何帧！\n";
               continue;
            }
            
            //ip.playback();
            //回放展示frames中保存的帧

            // 1. 图像转 base64
            vector<string> base64Imgs;
            for (auto& f : frames)
                base64Imgs.push_back(stai.matToBase64(f));
            */


            // 1. v4l2采集並轉換為base64
            vector<string> base64Imgs = v4l2_work();
            if(base64Imgs.empty())
            {
                cout<<"摄像头采集工作失败或未采集到數據"<<endl;
                continue;
            }
		
		cout<<"采集完成"<<endl;

            // 2. 组 JSON
            string jsonBody = stai.packToJson(base64Imgs, textPrompt);

		cout<<"json组建完成"<<endl;

		

            // 3. 发给 AI
            string aiResponse = stai.sendToAI(token, model, jsonBody);

		cout<<"发送成功，回应接收"<<endl;

            // 4. 显示文字
            string reply = extractAIResponse(aiResponse);
            //剖去json格式，只保留string类型回答内容
            
            // 直接輸出AI回應
            cout << "\n=== AI 回應 ===" << endl;
            cout << reply << endl;
            cout << "===============\n" << endl;
            
            /*opencv顯示方案（已註釋）
            // 先釋放v4l2資源，以便opencv可以訪問攝像頭用於顯示
            v4l2_free();
            
            // 初始化opencv用於顯示（如果還沒初始化的話）
            if(!ip.getCapture().isOpened()) {
                ip.init();
                if(!ip.getCapture().isOpened()) {
                    cout << "無法打開攝像頭用於顯示" << endl;
                    continue;
                }
            }
            
            // 將回答内容按30个字符一组切割並顯示
            op.showPagedText(ip.getCapture(), reply, 30, 5);
            
            // 重新初始化v4l2以供下次使用
            if(v4l2_init("/dev/video0") < 0) {
                cout << "重新初始化v4l2失敗" << endl;
                break;
            }
            */
        }
    }

    // 程序結束時清理資源
    v4l2_free();
    return 0;
}




















































