#include <curl/curl.h>
#include <opencv2/opencv.hpp>
#include<iostream>
#include <string>
#include <vector>

using namespace std;
using namespace cv;
using json = nlohmann::json;

class testdemo
{
public:
	string matToBase64(Mat& img);
	string packToJson(vector<string>& base64Imgs,
		 string& textPrompt);
	string sendToAI(string& token,string& model,string& aiRequestJson);
};
