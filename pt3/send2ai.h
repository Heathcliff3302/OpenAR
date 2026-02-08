#ifndef SEND2AI_H
#define SEND2AI_H

#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include <string>
#include <vector>

using namespace std;
using json = nlohmann::json;

class base64
{
public:
	string packToJson(vector<string>& base64Imgs,
		 string& textPrompt);
	string sendToAI(string& token,string& model,string& aiRequestJson);
};

#endif // BASE64_H
