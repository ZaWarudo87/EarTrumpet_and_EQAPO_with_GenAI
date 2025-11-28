#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <string>
#include <iostream>
#include <fstream>

using json = nlohmann::json;

// cURL callback
size_t WriteCallback(void *contents, size_t size, size_t nmemb, std::string *userp)
{
    size_t totalSize = size * nmemb;
    userp->append((char *)contents, totalSize);
    return totalSize;
}

// input requirement, return config(in JSON)
std::string GetEQConfigFromGemini(const std::string &user_prompt, const std::string &api_key)
{
    CURL *curl;
    CURLcode res;
    std::string readBuffer;

    curl = curl_easy_init();
    if (curl)
    {
        // API
        std::string url = "https://generativelanguage.googleapis.com/v1beta/models/gemini-2.5-pro:generateContent?key=" + api_key;

        // response body
        json payload;

        // enforce the response to json, and define the return format
        payload["generationConfig"] = {
            {"responseMimeType", "application/json"}};
        std::string schema_instruction =
            "You are an Equalizer APO config generator. "
            "Output ONLY a JSON object with this exact structure: "
            "{"
            "  \"preamp_db\": float (negative value to prevent clipping),"
            "  \"filters\": ["
            "    {\"type\": \"PK\"|\"LS\"|\"HS\", \"frequency_hz\": float, \"gain_db\": float, \"q_factor\": float}"
            "  ]"
            "}";
        // combine all together
        payload["contents"] = {
            {{"parts", {{{"text", schema_instruction + "\nUser Request: " + user_prompt}}}}}};

        std::string json_str = payload.dump();
        // setting curl header
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_str.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        // request
        res = curl_easy_perform(curl);
        // 清理
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        if (res != CURLE_OK)
        {
            return ""; // 錯誤處理
        }
    }

    // return format: { "candidates": [ { "content": { "parts": [ { "text": "XXX" } ] } } ] }
    return readBuffer;
}

// after getting config in JSON, parse and write into config.txt
void ParseAndWriteConfig(const std::string &raw_response)
{

    const std::string config_path = "config.txt"; /* TODO:之後要改實際修改的config位置 */

    try
    {
        // parse API return
        auto api_response = json::parse(raw_response);

        // checking network error
        if (!api_response.contains("candidates") || api_response["candidates"].empty())
        {
            std::cerr << "Error: No candidates found." << std::endl;
            return;
        }

        // get desired content
        std::string inner_json_text = api_response["candidates"][0]["content"]["parts"][0]["text"];

        // inner JSON parse
        auto eq_config = json::parse(inner_json_text);

        // write into file (overwrite)
        std::ofstream outFile(config_path, std::ios::out | std::ios::trunc);

        if (!outFile.is_open())
        {
            std::cerr << "Error: Could not open " << config_path << " for writing." << std::endl;
            std::cerr << "Check file permissions (Run as Administrator?)." << std::endl;
            return;
        }

        // 1. write preamp (usually negative)
        if (eq_config.contains("preamp_db"))
        {
            float preamp = eq_config["preamp_db"];
            outFile << "Preamp: " << preamp << " dB" << std::endl;
        }

        // 2. write filter
        // format: Filter: ON PK Fc 100 Hz Gain 3.0 dB Q 1.4
        if (eq_config.contains("filters") && eq_config["filters"].is_array())
        {
            for (const auto &filter : eq_config["filters"])
            {
                std::string type = filter["type"];
                float freq = filter["frequency_hz"];
                float gain = filter["gain_db"];
                float q = filter["q_factor"];

                // combine and write in
                outFile << "Filter: ON " << type
                        << " Fc " << freq << " Hz"
                        << " Gain " << gain << " dB"
                        << " Q " << q << std::endl;
            }
        }
        outFile.close();

        std::cout << "Successfully wrote config to " << config_path << std::endl;
    }
    catch (const json::exception &e)
    {
        std::cerr << "JSON Parsing Error: " << e.what() << std::endl;
    }
    catch (const std::ios_base::failure &e)
    {
        std::cerr << "File I/O Error: " << e.what() << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Unknown Error: " << e.what() << std::endl;
    }
}
int main()
{
    std::string raw_response = GetEQConfigFromGemini(u8"我要適合聽人聲的設定", "AIzaSyDTqRZspN7Xc_vDgYXjdwoWfN62LxdbWe8");
    ParseAndWriteConfig(raw_response);

    auto response_json = json::parse(raw_response);
}