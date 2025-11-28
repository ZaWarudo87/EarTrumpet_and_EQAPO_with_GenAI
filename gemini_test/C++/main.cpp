#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <string>
#include <iostream>
#include <fstream>
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
#include <conio.h>

using json = nlohmann::json;

static const std::string base64_chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

// file to base 64 (By GPT)
std::string FileToBase64(const std::string &filepath)
{
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open())
        return "";

    // store content
    std::vector<unsigned char> buffer((std::istreambuf_iterator<char>(file)),
                                      std::istreambuf_iterator<char>());
    std::string encoded_string;
    int i = 0, j = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];

    for (unsigned char byte : buffer)
    {
        char_array_3[i++] = byte;
        if (i == 3)
        {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for (i = 0; (i < 4); i++)
                encoded_string += base64_chars[char_array_4[i]];
            i = 0;
        }
    }

    if (i)
    {
        for (j = i; j < 3; j++)
            char_array_3[j] = '\0';
        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        for (j = 0; (j < i + 1); j++)
            encoded_string += base64_chars[char_array_4[j]];
        while ((i++ < 3))
            encoded_string += '=';
    }

    return encoded_string;
}

// recording sounds with miniAudio
void data_callback(ma_device *pDevice, void *pOutput, const void *pInput, ma_uint32 frameCount)
{
    ma_encoder *pEncoder = (ma_encoder *)pDevice->pUserData;
    if (pEncoder == NULL)
        return;

    // write the mic input data to output file
    ma_encoder_write_pcm_frames(pEncoder, pInput, frameCount, NULL);
}

// main function for recording
// press enter to stopping recording
bool RecordUserAudio(const std::string &outputFilename)
{
    ma_result result;
    ma_encoder_config encoderConfig;
    ma_encoder encoder;
    ma_device_config deviceConfig;
    ma_device device;

    // initialize wav encoder
    encoderConfig = ma_encoder_config_init(ma_encoding_format_wav, ma_format_f32, 1, 44100);
    if (ma_encoder_init_file(outputFilename.c_str(), &encoderConfig, &encoder) != MA_SUCCESS)
    {
        std::cerr << "Failed to initialize output file." << std::endl;
        return false;
    }

    // initialize microphone device
    deviceConfig = ma_device_config_init(ma_device_type_capture);
    deviceConfig.capture.format = encoder.config.format;
    deviceConfig.capture.channels = encoder.config.channels;
    deviceConfig.sampleRate = encoder.config.sampleRate;
    deviceConfig.dataCallback = data_callback;
    deviceConfig.pUserData = &encoder; // encoder to callback

    if (ma_device_init(NULL, &deviceConfig, &device) != MA_SUCCESS)
    {
        std::cerr << "Failed to initialize capture device." << std::endl;
        ma_encoder_uninit(&encoder);
        return false;
    }

    // start recording
    if (ma_device_start(&device) != MA_SUCCESS)
    {
        std::cerr << "Failed to start device." << std::endl;
        ma_device_uninit(&device);
        ma_encoder_uninit(&encoder);
        return false;
    }

    std::cout << "Recording... Press ENTER to stop." << std::endl;

    // wait for enter
    while (true)
    {
        if (_kbhit())
        {
            if (_getch() == 13)
                break; // 13 for Enter
        }
        ma_sleep(10);
    }

    // release resource
    ma_device_uninit(&device);
    ma_encoder_uninit(&encoder);

    std::cout << "Recording saved to " << outputFilename << std::endl;
    return true;
}

// cURL callback
size_t WriteCallback(void *contents, size_t size, size_t nmemb, std::string *userp)
{
    size_t totalSize = size * nmemb;
    userp->append((char *)contents, totalSize);
    return totalSize;
}

// input requirement, return config(in JSON)
std::string GetEQConfigFromGemini(const std::string &api_key)
{
    CURL *curl;
    CURLcode res;
    std::string readBuffer;

    std::string audio_file = "command.wav";
    if (!RecordUserAudio(audio_file))
        return "Recording Error";

    // transfer code to base64 for sending
    std::string audio_base64 = FileToBase64(audio_file);
    if (audio_base64.empty())
        return "Encoding Error";

    curl = curl_easy_init();
    if (curl)
    {
        // API
        std::string url = "https://generativelanguage.googleapis.com/v1beta/models/gemini-2.5-pro:generateContent?key=" + api_key;

        // response body
        json payload;

        // enforce the response to json, and define the return format (by GPT)
        payload["generationConfig"] = {
            {"responseMimeType", "application/json"}};
        std::string schema_instruction =
            "You are an audio engineer. Listen to the user's voice command. "
            "Based on the request, generate an Equalizer APO config JSON object. "
            "WARNING: Output ONLY a JSON object with this exact structure: "
            "{"
            "  \"preamp_db\": float (negative value to prevent clipping),"
            "  \"filters\": ["
            "    {\"type\": \"PK\"|\"LS\"|\"HS\", \"frequency_hz\": float, \"gain_db\": float, \"q_factor\": float}"
            "  ]"
            "}";
        ;
        // combine all together
        payload["contents"] = {
            {{"parts", {// prompt content
                        {{"text", schema_instruction}},

                        // audio content (in base 64)
                        {{"inlineData", {{"mimeType", "audio/wav"}, {"data", audio_base64}}}}}}}};

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

        // release resource
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        if (res != CURLE_OK)
        {
            return "";
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
    std::string raw_response = GetEQConfigFromGemini("AIzaSyDTqRZspN7Xc_vDgYXjdwoWfN62LxdbWe8");
    ParseAndWriteConfig(raw_response);

    auto response_json = json::parse(raw_response);
}