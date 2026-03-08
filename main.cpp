#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include "send2ai.h"
#include "camera.h"
#include "drm.h"
#include "audio.h"
#include <thread>
#include <mutex>
#include <condition_variable>

using namespace std;
using json = nlohmann::json;

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

    cout << "按下s开始录制采集，按 q 退出" << endl;

    if (v4l2_init("/dev/video0") < 0)
    {
        cout << "摄像头初始化失败" << endl;
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
            cout << "开始录制...\n";

            {
                std::lock_guard<std::mutex> lk(mtx);
                image_done = false;
                audio_done = false;
                g_base64Imgs.clear();
                g_textPrompt.clear();
            }

            // 图像采集线程：采集图像转换数据后写入g_base64Imgs，然后置image_done为true
            std::thread t_image([&]() {
                vector<string> imgs = v4l2_work();
                {
                    std::lock_guard<std::mutex> lk(mtx);
                    g_base64Imgs = std::move(imgs);
                    image_done = true;
                    cv.notify_all();
                }
            });

            // 语音采集线程：采集语音whisper识别后写入g_textPrompt，然后置audio_done为true
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

            // 主线程：等待两路完成信号
            {
                std::unique_lock<std::mutex> lk(mtx);
                cv.wait(lk, [] { return image_done && audio_done; });
            }

            t_image.join();
            t_audio.join();

            if (g_base64Imgs.empty())
            {
                cout << "摄像头数据采集失败" << endl;
                continue;
            }   

            cout << "图像+语音数据采集完成" << endl;

            // 用图像数据和语音数据组建json请求
            string jsonBody = stai.packToJson(g_base64Imgs, g_textPrompt);
		    cout<<"json组建完成"<<endl;
            		
            
            // 3. 调用curl库发给 AI
            string aiResponse = stai.sendToAI(token, model, jsonBody);

		    cout<<"发送成功，回应接收"<<endl;

            // 4. 基于DRM-GBM-EGL显示文字
            string reply = stai.extractAIResponse(aiResponse);
            //剖去json格式，只保留string类型回答内容
            drm.drmdisplay(reply.c_str());
            //pt3,DRM直接显示
 
        }
    }

    // 资源释放（摄像头）
    v4l2_free();
    return 0;
}




















































