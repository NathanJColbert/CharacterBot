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
    CharacterSettings(const char* characterP, long long minResponse, size_t minBuffer, long long minSpokenTimeout, long long emptyTimeout) :
    characterPrompt(characterP), minResponseTime(minResponse), minBufferSize(minBuffer), minSpokenTimeout(minSpokenTimeout), emptySpaceTimeout(emptyTimeout)
    { }
    std::string characterPrompt;
	long long minResponseTime;
	size_t minBufferSize;
    long long minSpokenTimeout;
    long long emptySpaceTimeout;
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
	BotInformation(const ServiceInformation& serviceInfo, const CharacterSettings& characterSet, const AudioReceiverSettings& audioReceiver, long long checkTime) :
	serviceInformation(serviceInfo), characterSettings(characterSet), audioReceiverSettings(audioReceiver), checkTimeMilliseconds(checkTime)
	{ }
	ServiceInformation serviceInformation;
	CharacterSettings characterSettings;
	AudioReceiverSettings audioReceiverSettings;
    long long checkTimeMilliseconds;
};

/*
A single guild object for voice connection.
Each guild can have exactly one connected voice.
This should be thread safe...
*/
class GuildInformation {
public:
	GuildInformation(const dpp::snowflake& guild, dpp::voiceconn* conn, BotInformation* information) :
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
	
    ~GuildInformation() { stop(); }

    /*
    Stops the current threads in the object.
    This is called from the deconstructor.
    */
    void stop() {
        running = false;
        if (workerThread.joinable())
            workerThread.join();
        if (voiceThread.joinable())
            voiceThread.join();
    }

    /*
    Sends the dpp voice data to the guild object.
    */
    bool sendVoiceData(const dpp::voice_receive_t& data);

    /*
    Adds a new user to the guild object in the call
    */
    bool addUser(const dpp::snowflake& user, const std::string& userName);
    /*
    Removes a user from the guild object in the call
    */
    bool removeUser(const dpp::snowflake& user);

    size_t userCount() const { return usersReceivers.size(); }
    dpp::voiceconn* getConnection() { return connection; }

private:
    BotInformation* botInformation;

    dpp::snowflake guildId;
    std::unordered_map<dpp::snowflake, std::string> userNames;
    std::unordered_map<dpp::snowflake, std::unique_ptr<AudioReceiver>> usersReceivers;
    SpeechToText speechToText;
	ServiceHandler serviceHandler;
	dpp::voiceconn* connection;

    std::mutex accessLock;
    std::atomic<bool> running;
    std::thread workerThread;

    std::thread voiceThread;
    std::atomic<bool> isVoiceThreadRunning{false};

    std::mutex userPromptLock;
    std::string userPrompt;

    std::chrono::steady_clock::time_point lastResponse;
    std::mutex lastResponseLock;
    
    std::chrono::steady_clock::time_point lastUserSpokenTime;
    std::mutex lastUserSpokenTimeLock;

    std::mutex voiceSendMutex;

    void timeoutLoop();
    void updateTimeout();
    void processNewSpeechToText(std::vector<int16_t>* buffer, const dpp::snowflake& user);
    void sendPrompt();
    void streamPCMToVoiceChannel(std::vector<uint8_t>& pcm_data);

    std::vector<uint8_t> upsampleAndConvert(const std::vector<int16_t>& input);
    void savePCMToFile(const std::vector<int16_t>& audioData, const std::string& filename);
};