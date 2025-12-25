#include <vector>
#include <sstream>
#include <fstream>
#include <nlohmann/json.hpp>
#include <curl/curl.h>
#include <codecvt>
#include <locale>
#include "send2ai.h"


using namespace std;

using json = nlohmann::json;
static size_t CurlWriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t totalSize = size * nmemb;
    std::string* response = static_cast<std::string*>(userp);
    response->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}


std::string base64::packToJson(std::vector<std::string>& base64Imgs,
    std::string& textPrompt)
{
    json j;
    //j["model"] = "Qwen/Qwen3-VL-32B-Thinking";

    json contentItems = json::array();
    
    // ����ͼƬ
    for (const auto& b64 : base64Imgs) {
        json imgObj;
        imgObj["type"] = "image_url";
        json imgUrlObj;
        imgUrlObj["url"] = "data:image/jpeg;base64," + b64;
        imgUrlObj["detail"] = "high";
        imgObj["image_url"] = imgUrlObj;
        contentItems.push_back(imgObj);
    }
    //给每张图片添加type，然后用imgUrl包装成需要的json格式

    //将提示词包装成需要的json格式
    json textObj;
    textObj["text"] = textPrompt;
    textObj["type"] = "text";
    contentItems.push_back(textObj);

    //将图片和提示词整体封装成一个最终的messages名称的json格式请求
    // Messages
    j["messages"] = json::array({
        {
        { "role","user" }
        ,
        { "content", contentItems }
        }
        });
    return j.dump();
    //这里其实返回的只是一个json格式排列的string文本
}

// 3. 将json发送给AI
string base64::sendToAI(string& token,string& model,string& aiRequestJson)
{
    // 解析请求JSON，添加model字段
    json payload;
    try {
        payload = json::parse(aiRequestJson);
        //传入一个json格式排序的string类型，格式没问题直接转为json，有问题的话传出去给catch解析错误
        payload["model"] = model;
    }
    catch (const std::exception& e) {
        return "{\"error\": \"Invalid JSON format: " + std::string(e.what()) + "\"}";
    }

    std::string url = "https://api.siliconflow.cn/v1/chat/completions";
    std::string response;

    CURL* curl = curl_easy_init();
    //初始化，创建一个curl对象，并返回一个句柄curl
    if (!curl) {
        return "{\"error\": \"curl init failed\"}";
    }

    struct curl_slist* headers = nullptr;
    std::string authHeader = std::string("Authorization: Bearer ") + token;
    //http请求头Authorization: Bearer+token
    headers = curl_slist_append(headers, authHeader.c_str());
    //用curl_slist_append函数将http请求头添加上Content-Type: application/json
    headers = curl_slist_append(headers, "Content-Type: application/json");
    //将http请求头添加到curl对象中

    std::string body = payload.dump();
    //将json对象序列化为string

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    //将返回的内容存到response中
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    //完成请求头设置，开始发送请求

    CURLcode res = curl_easy_perform(curl);
    //三次握手，发送报文，接收响应
    //这里的res只是告诉你网络通畅，内容是存到response的
    if (res != CURLE_OK) {
        response = "{\"error\": \"curl perform failed: " + std::string(curl_easy_strerror(res)) + "\"}";
    }

    curl_slist_free_all(headers);
    //释放请求头
    curl_easy_cleanup(curl);
    //释放curl对象
    return response;
}
