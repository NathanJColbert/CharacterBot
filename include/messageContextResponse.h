#pragma once
#include <dpp/dpp.h>
#include <serviceHandler.h>
#include <algorithm>
#include <random>

struct MessageContextSettings {
    MessageContextSettings(const char* openAI, const char* prompt, const std::vector<std::string> keys, float chance) :
        openAIKey(openAI), characterPrompt(prompt), keywords(keys), responseChance(chance) { }
    std::string openAIKey;
    std::string characterPrompt;
    std::vector<std::string> keywords;
    float responseChance;
};

class MessageContextResponse {
public:
    MessageContextResponse(MessageContextSettings* settings) : 
        contextSettings(settings), handler(settings->openAIKey, "", "") { }

    bool tryGetResponse(const dpp::message_create_t& event, dpp::message& response) {
        if (event.msg.content.empty()) {
            //std::cout << "Content is empty..." << std::endl;
            return false;
        }
        if (event.msg.content.size() > 200) return false;

        if (!containsKeyword(event.msg.content, contextSettings->keywords) && !roll(contextSettings->responseChance))
            return false;
        
        std::string sendTo = event.msg.author.username + ", \"" + event.msg.content + "\"";
        std::string result = handler.openAiResponse(sendTo, contextSettings->characterPrompt);
        response = dpp::message(event.msg.channel_id, result);
        return true;
    }

private:
    MessageContextSettings* contextSettings;
    ServiceHandler handler;

    bool containsKeyword(const std::string& msg, const std::vector<std::string> keywords) {
        std::string lower_msg = msg;
        std::transform(lower_msg.begin(), lower_msg.end(), lower_msg.begin(), [](unsigned char c) {
            return std::tolower(c);
        });
        
        for (const auto& keyword : keywords) {
            if (lower_msg.find(keyword) != std::string::npos)
                return true;
        }
        return false;
    }

    bool roll(float chance) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dist(0.0f, 100.0f);

        return dist(gen) < chance;
    }
};