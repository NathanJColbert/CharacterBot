#pragma once
#include <unordered_map>
#include <dpp/dpp.h>
#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>
#include <mutex>
#include "audioReceiver.h"
#include "speechToText.h"
#include "serviceHandler.h"

struct CharacterSettings {
    CharacterSettings(const char* characterP, long long minResponse, size_t minBuffer) :
    characterPrompt(characterP), minResponseTime(minResponse), minBufferSize(minBuffer)
    { }
    std::string characterPrompt;
	long long minResponseTime;
	size_t minBufferSize;
};

struct ServiceInformation {
	ServiceInformation() = default;
	ServiceInformation(const char* openAI, const char* leopard, const char* elevenLab, const char* elevenLabId) :
	openAIToken(openAI), leopardToken(leopard), elevenLabsToken(elevenLab), elevenLabsSpeechId(elevenLabId)
	{ }
	std::string openAIToken;
	std::string leopardToken;
	std::string elevenLabsToken;
	std::string elevenLabsSpeechId;
};

struct BotInformation {
	BotInformation() = default;
	BotInformation(const ServiceInformation& serviceInfo, const CharacterSettings& characterSet, const AudioReceiverSettings& audioReceiver) :
	serviceInformation(serviceInfo), characterSettings(characterSet), audioReceiverSettings(audioReceiver)
	{ }
	ServiceInformation serviceInformation;
	CharacterSettings characterSettings;
	AudioReceiverSettings audioReceiverSettings;
};

class GuildInformation {
public:
	GuildInformation(const dpp::snowflake& guild, dpp::discord_client* conn, BotInformation* information) :
    guildId(guild), connection(conn), botInformation(information), 
    speechToText(guild, information->serviceInformation.leopardToken.c_str()), 
    serviceHandler(
        information->serviceInformation.openAIToken.c_str(), 
        information->serviceInformation.elevenLabsToken.c_str(),
        information->serviceInformation.elevenLabsSpeechId), 
    running(true) {
	    usersReceivers.reserve(20);
        userPrompt.reserve(1000);
        workerThread = std::thread(&GuildInformation::timeoutLoop, this);
    }
	
    ~GuildInformation() {
        running = false;
        if (workerThread.joinable())
            workerThread.join();
        for (auto i = usersReceivers.begin(); i != usersReceivers.end(); i++)
            if (i->second != NULL) delete i->second;
    }

    void stop() {
        running = false;
        if (workerThread.joinable())
            workerThread.join();
    }

    bool sendVoiceData(const dpp::voice_receive_t& data) {
        AudioReceiver* receiver = NULL;
        {
            std::lock_guard<std::mutex> lock(accessLock);
            auto it = usersReceivers.find(data.user_id);
            if (it == usersReceivers.end())
                return false;
            receiver = it->second;
        }
        receiver->onVoiceReceive(data);
		return true;
    }

    bool addUser(const dpp::snowflake& user, const std::string& userName) {
        //std::cout << "Add User: Accessing lock\n";
        std::lock_guard<std::mutex> lock(accessLock);
        //std::cout << "Add User: Lock Accessed\n";
	    auto userIterator = usersReceivers.find(user);
	    if (userIterator != usersReceivers.end())
		    return false;
	    userNames.insert(std::pair(user, userName));
        usersReceivers.insert(std::pair(user, new AudioReceiver(user, &(botInformation->audioReceiverSettings))));
	    return true;
    }
    bool removeUser(const dpp::snowflake& user) {
        //std::cout << "Remove User: Accessing lock\n";
        std::lock_guard<std::mutex> lock(accessLock);
        //std::cout << "Remove User: Lock Accessed\n";

        auto userNameMap = userNames.find(user);
        if (userNameMap != userNames.end())
            userNames.erase(userNameMap);

        auto receiverMap = usersReceivers.find(user);
	    if (receiverMap == usersReceivers.end())
		    return false;
		delete receiverMap->second;
        usersReceivers.erase(receiverMap);
		return true;
	}

    size_t userCount() const { return usersReceivers.size(); }
    dpp::discord_client* getConnection() { return connection; }

private:
    BotInformation* botInformation;

    dpp::snowflake guildId;
    std::unordered_map<dpp::snowflake, std::string> userNames;
    std::unordered_map<dpp::snowflake, AudioReceiver*> usersReceivers;
    SpeechToText speechToText;
	ServiceHandler serviceHandler;
	dpp::discord_client* connection;

    std::mutex accessLock;
    std::atomic<bool> running;
    std::thread workerThread;

    std::thread voiceThread;
    std::atomic<bool> isVoiceThreadRunning{false};

    std::string userPrompt;

    std::chrono::steady_clock::time_point lastResponse;
    std::mutex lastResponseLock;

    void timeoutLoop() {
        std::cout << "Guild thread started for " << guildId << std::endl;
        while (running) {
            updateTimeout();
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
        std::cout << "Guild thread exiting for " << guildId << std::endl;
    }

    void updateTimeout() {
        {
            std::lock_guard<std::mutex> lock(accessLock);
	        for (auto user = usersReceivers.begin(); user != usersReceivers.end(); user++) {
			    std::vector<int16_t> buffer;
                if (user->second->checkTimeout(buffer) && !buffer.empty()) {
				    auto userSnowflake = user->first;
				    processNewSpeechToText(&buffer, user->first);
			    }
		    }
        }
        std::chrono::steady_clock::time_point snapshot;
        {
            std::lock_guard<std::mutex> lock(lastResponseLock);
            snapshot = lastResponse;
        }
        auto now = std::chrono::steady_clock::now();
	    auto lastResponseDuration = std::chrono::duration_cast<std::chrono::milliseconds>(now - snapshot).count();
        if (userPrompt.size() < botInformation->characterSettings.minBufferSize) return;
        if (lastResponseDuration < botInformation->characterSettings.minResponseTime) {
            //std::cout << "Prevented premature response at -> " << lastResponseDuration << " milliseconds. Needed -> " << MIN_TIME_BETWEEN_RESPONSES_MILLISECONDS << " milliseconds.\n";
            return;
        }
        
        bool expected = false;
        if (!isVoiceThreadRunning.compare_exchange_strong(expected, true))
            return;
        voiceThread = std::thread([this] {
            {
            std::lock_guard<std::mutex> lock(lastResponseLock);
            lastResponse = std::chrono::steady_clock::now();
            }
            this->sendPrompt();
            isVoiceThreadRunning.store(false);
        });
        voiceThread.detach();
    }

    void processNewSpeechToText(std::vector<int16_t>* buffer, const dpp::snowflake& user) {
        std::string stt = speechToText.getResponse(buffer, user);
        if (stt.empty()) return;
	    std::cout << stt << std::endl;

        std::string name = "";
        auto userName = userNames.find(user);
        if (userName != userNames.end()) name = userName->second;
        std::string newPrompt;
        newPrompt.reserve(100);
        newPrompt.append(name).append(", \"").append(stt).append("\"\n");
        userPrompt.append(std::move(newPrompt));

        std::cout << "Added new object to list.\n";
    }

    std::vector<uint8_t> upsampleAndConvert(const std::vector<int16_t>& input) {
        std::vector<uint8_t> output;
        output.reserve(input.size() * 4 * sizeof(int16_t));

        const float volume_scale = 0.9f;

        for (size_t i = 0; i < input.size() - 1; ++i) {
            int16_t a = static_cast<int16_t>(input[i] * volume_scale);
            int16_t b = static_cast<int16_t>(input[i + 1] * volume_scale);

            int16_t s1 = a;
            int16_t s2 = static_cast<int16_t>((a + b) / 2);

            output.push_back(s1 & 0xFF);             // LSB
            output.push_back((s1 >> 8) & 0xFF);      // MSB
            output.push_back(s1 & 0xFF);
            output.push_back((s1 >> 8) & 0xFF);

            output.push_back(s2 & 0xFF);
            output.push_back((s2 >> 8) & 0xFF);
            output.push_back(s2 & 0xFF);
            output.push_back((s2 >> 8) & 0xFF);
        }

        int16_t last = static_cast<int16_t>(input.back() * volume_scale);
        output.push_back(last & 0xFF);
        output.push_back((last >> 8) & 0xFF);
        output.push_back(last & 0xFF);
        output.push_back((last >> 8) & 0xFF);

        return output;
    }
    void sendPrompt() {
        std::cout << "- NEW PROMPT WILL BE SENT -\n";
        std::cout << userPrompt << '\n';

        std::string result = serviceHandler.openAiResponse(userPrompt, botInformation->characterSettings.characterPrompt);
        std::cout << "- RESULT -\n";
        std::cout << result << '\n';

        std::vector<int16_t> audioResult = serviceHandler.elevenLabsTextToSpeech(result);
        //savePCMToFile(audioResult, "TMP.pcm");
        auto test = upsampleAndConvert(audioResult);
        streamPCMToVoiceChannel(test);
        userPrompt.clear();
    }

    void streamPCMToVoiceChannel(std::vector<uint8_t>& pcm_data) {
        if (!connection) {
            std::cout << "Bot cluster not initialized.\n";
            return;
        }

        dpp::voiceconn* conn = connection->get_voice(guildId);
        if (!conn || !conn->voiceclient) {
            std::cout << "Invalid voice connection.\n";
            return;
        }

        int remainder = pcm_data.size() % 4;
        if (remainder != 0) {
            std::cout << "Trimming PCM buffer for alignment.\n";
            for (int i = 0; i < remainder; i++) pcm_data.pop_back();
        }

        conn->voiceclient->send_audio_raw(reinterpret_cast<uint16_t*>(pcm_data.data()), pcm_data.size());
    }

    void savePCMToFile(const std::vector<int16_t>& audioData, const std::string& filename) {
        std::ofstream outFile(filename, std::ios::binary);
        if (!outFile) {
            std::cerr << "Failed to open file for writing: " << filename << std::endl;
            return;
        }

        outFile.write(reinterpret_cast<const char*>(audioData.data()), audioData.size() * sizeof(int16_t));
        outFile.close();
        std::cout << "Saved " << audioData.size() << " samples to " << filename << std::endl;
    }
};