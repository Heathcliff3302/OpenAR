#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include "send2ai.h"
#include "v4l2.h"
#include "drm.h"
#include "audio.h"
#include <thread>
#include <mutex>
#include <condition_variable>

using namespace std;
using json = nlohmann::json;

// 雙線程完成信號：主線程等待圖像與語音兩路均完成後再合併 JSON
static std::mutex mtx;
static std::condition_variable cv;
static bool image_done = false;
static bool audio_done = false;
static vector<string> g_base64Imgs;
static string g_textPrompt;

int main()
{
    base64 stai;
    drmclass drm;
    audio adio;

    string token = "sk-tniblsjfylvkfquhcbzaymwnwmkmgitwhakqycgwwskhctwp";
    string model = "Qwen/Qwen3-VL-32B-Thinking";

    cout << "按下 s 開始錄製（圖像+語音同時採集 10s），按 q 退出" << endl;

    if (v4l2_init("/dev/video0") < 0)
    {
        cout << "攝像頭初始化失敗" << endl;
        return -1;
    }

    while (true)
    {
        char c = getchar();
        if (c == '\n') continue;

        if (c == 'q' || c == 'Q') {
            cout << "退出程序\n";
            break;
        }

        if (c == 's' || c == 'S')
        {
            cout << "開始錄製（圖像與語音同時採集 10 秒）...\n";

            {
                std::lock_guard<std::mutex> lk(mtx);
                image_done = false;
                audio_done = false;
                g_base64Imgs.clear();
                g_textPrompt.clear();
            }

            // 線程 1：圖像採集 v4l2_work()，採集約 10s 後寫入 g_base64Imgs 並置完成
            std::thread t_image([&]() {
                vector<string> imgs = v4l2_work();
                {
                    std::lock_guard<std::mutex> lk(mtx);
                    g_base64Imgs = std::move(imgs);
                    image_done = true;
                    cv.notify_all();
                }
            });

            // 線程 2：語音採集與識別 audio_thread()，結果寫入 textPrompt 並置完成
            std::thread t_audio([&]() {
                string textPrompt;
                adio.conversion(&textPrompt);
                {
                    std::lock_guard<std::mutex> lk(mtx);
                    g_textPrompt = std::move(textPrompt);
                    audio_done = true;
                    cv.notify_all();
                }
            });

            // 主線程：等待兩路完成信號
            {
                std::unique_lock<std::mutex> lk(mtx);
                cv.wait(lk, [] { return image_done && audio_done; });
            }

            t_image.join();
            t_audio.join();

            if (g_base64Imgs.empty())
            {
                cout << "攝像頭採集失敗或未採集到數據" << endl;
                continue;
            }

            cout << "採集完成（圖像+語音）" << endl;

            // 使用線程結果組 JSON（圖像 g_base64Imgs + 語音 g_textPrompt）
            string jsonBody = stai.packToJson(g_base64Imgs, g_textPrompt);
		    cout<<"json组建完成"<<endl;
            		
            
            // 3. 发给 AI
            string aiResponse = stai.sendToAI(token, model, jsonBody);

		    cout<<"发送成功，回应接收"<<endl;

            // 4. 顯示文字
            string reply = stai.extractAIResponse(aiResponse);
            //剖去json格式，只保留string类型回答内容
            drm.drmdisplay(reply.c_str());
            //pt3,DRM直接显示
            /*
            // 直接輸出AI回應
            cout << "\n=== AI 回應 ===" << endl;
            cout << reply << endl;
            cout << "===============\n" << endl;
            */
        }
    }

    // 程序結束時清理資源
    v4l2_free();
    return 0;
}




















































