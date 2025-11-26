#include <opencv2/opencv.hpp>
#include <vector>
#include <sstream>
#include <fstream>
#include <nlohmann/json.hpp>
#include <curl/curl.h>
#include <codecvt>
#include <locale>
#include "base64.h"


using namespace cv;
using namespace std;

using json = nlohmann::json;
static size_t CurlWriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t totalSize = size * nmemb;
    std::string* response = static_cast<std::string*>(userp);
    response->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}


// ����ͼƬתΪBase64+JSON
string testdemo::matToBase64(Mat& img)
{
    std::vector<uchar> buf;
    cv::imencode(".jpg", img, buf);

    static const char base64_chars[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";

    std::string ret;
    int i = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];

    std::vector<uchar>::const_iterator it = buf.begin();
    while (it != buf.end()) {
        char_array_3[i++] = *(it++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) +
                ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) +
                ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for (i = 0; i < 4; i++)
                ret += base64_chars[char_array_4[i]];
            i = 0;
        }
    }

    if (i) {
        for (int j = i; j < 3; j++)
            char_array_3[j] = 0;

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) +
            ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) +
            ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;

        for (int j = 0; j < i + 1; j++)
            ret += base64_chars[char_array_4[j]];

        while (i++ < 3)
            ret += '=';
    }

    return ret;
}

std::string testdemo::packToJson(std::vector<std::string>& base64Imgs,
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
    
    // �����ı�
    json textObj;
    textObj["text"] = textPrompt;
    textObj["type"] = "text";
    contentItems.push_back(textObj);

    // Messages
    j["messages"] = json::array({
        {
        { "role","user" }
        ,
        { "content", contentItems }
        }
        });
    return j.dump();
}

// 3. ���͸� AI����� curl POST��
string testdemo::sendToAI(string& token,string& model,string& aiRequestJson)
{
    // 解析请求JSON，添加model字段
    json payload;
    try {
        payload = json::parse(aiRequestJson);
        payload["model"] = model;
    }
    catch (const std::exception& e) {
        return "{\"error\": \"Invalid JSON format: " + std::string(e.what()) + "\"}";
    }

    std::string url = "https://api.siliconflow.cn/v1/chat/completions";
    std::string response;

    CURL* curl = curl_easy_init();
    if (!curl) {
        return "{\"error\": \"curl init failed\"}";
    }

    struct curl_slist* headers = nullptr;
    std::string authHeader = std::string("Authorization: Bearer ") + token;
    headers = curl_slist_append(headers, authHeader.c_str());
    headers = curl_slist_append(headers, "Content-Type: application/json");

    std::string body = payload.dump();

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        response = "{\"error\": \"curl perform failed: " + std::string(curl_easy_strerror(res)) + "\"}";
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    return response;
}
