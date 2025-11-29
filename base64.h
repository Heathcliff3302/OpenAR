#ifndef BASE64_H
#define BASE64_H

#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <opencv2/opencv.hpp>
#include <iostream>
#include <string>
#include <vector>

using namespace std;
using namespace cv;
using json = nlohmann::json;

class base64
{
public:
	string matToBase64(Mat& img);
	string packToJson(vector<string>& base64Imgs,
		 string& textPrompt);
	string sendToAI(string& token,string& model,string& aiRequestJson);
};

#endif // BASE64_H
