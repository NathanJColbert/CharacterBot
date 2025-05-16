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

class GuildInformation {
public:
	GuildInformation(const dpp::snowflake& guild, dpp::voiceconn* conn, const char* openAIKey, const char* leopardKey) :
    guildId(guild), connection(conn), speechToText(guild, leopardKey), serviceHandler(openAIKey), running(true) {
	    usersReceivers.reserve(20);
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

    bool addUser(const dpp::snowflake& user) {
        std::cout << "Add User: Accessing lock\n";
        std::lock_guard<std::mutex> lock(accessLock);
        std::cout << "Add User: Lock Accessed\n";
	    auto userIterator = usersReceivers.find(user);
	    if (userIterator != usersReceivers.end())
		    return false;
	    usersReceivers.insert(std::pair(user, new AudioReceiver(user)));
	    return true;
    }
    bool removeUser(const dpp::snowflake& user) {
        std::cout << "Remove User: Accessing lock\n";
        std::lock_guard<std::mutex> lock(accessLock);
        std::cout << "Remove User: Lock Accessed\n";
        auto receiverMap = usersReceivers.find(user);
	    if (receiverMap == usersReceivers.end())
		    return false;
		delete receiverMap->second;
		usersReceivers.erase(receiverMap);
		return true;
	}

    size_t userCount() const { return usersReceivers.size(); }
    
private:
    dpp::snowflake guildId;
    std::unordered_map<dpp::snowflake, AudioReceiver*> usersReceivers;
    SpeechToText speechToText;
	ServiceHandler serviceHandler;
	dpp::voiceconn* connection;

    std::unordered_map<dpp::snowflake, std::vector<std::string>> userSTT;
    std::mutex accessLock;
    std::atomic<bool> running;
    std::thread workerThread;

    void timeoutLoop() {
        std::cout << "Guild thread started for " << guildId << std::endl;
        while (running) {
            updateTimeout();
            std::this_thread::sleep_for(std::chrono::milliseconds(400));
        }
        std::cout << "Guild thread exiting for " << guildId << std::endl;
    }

    void updateTimeout() {
        std::lock_guard<std::mutex> lock(accessLock);
	    for (auto user = usersReceivers.begin(); user != usersReceivers.end(); user++) {
			std::vector<int16_t> buffer;
            if (user->second->checkTimeout(buffer) && !buffer.empty()) {
				auto userSnowflake = user->first;
				processNewSpeechToText(&buffer, user->first);
			}
		}
    }

    void processNewSpeechToText(std::vector<int16_t>* buffer, const dpp::snowflake& user) {
        std::string stt = speechToText.getResponse(buffer, user);
	    std::cout << stt << std::endl;
        
        auto& list = userSTT[user];
        if (list.empty()) list.reserve(20);
        list.emplace_back(stt);
        std::cout << "Added new object to list.\n";
    }
};