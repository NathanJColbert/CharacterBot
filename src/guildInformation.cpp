#include "guildInformation.h"

bool GuildInformation::sendVoiceData(const dpp::voice_receive_t& data) {
    {
        std::lock_guard<std::mutex> lock(lastUserSpokenTimeLock);
        lastUserSpokenTime = std::chrono::steady_clock::now();
    }

    AudioReceiver* receiver = nullptr;
    {
        std::lock_guard<std::mutex> lock(accessLock);
        auto it = usersReceivers.find(data.user_id);
        if (it == usersReceivers.end())
            return false;
        receiver = it->second.get();
    }
    receiver->onVoiceReceive(data);
	return true;
}

bool GuildInformation::addUser(const dpp::snowflake& user, const std::string& userName) {
    //std::cout << "Add User: Accessing lock\n";
    std::lock_guard<std::mutex> lock(accessLock);
    //std::cout << "Add User: Lock Accessed\n";
	auto userIterator = usersReceivers.find(user);
	if (userIterator != usersReceivers.end())
		return false;
	userNames.insert(std::pair(user, userName));
    usersReceivers.emplace(user, std::make_unique<AudioReceiver>(user, &(botInformation->audioReceiverSettings)));
	return true;
}
bool GuildInformation::removeUser(const dpp::snowflake& user) {
    //std::cout << "Remove User: Accessing lock\n";
    std::lock_guard<std::mutex> lock(accessLock);
    //std::cout << "Remove User: Lock Accessed\n";

    auto userNameMap = userNames.find(user);
    if (userNameMap != userNames.end())
        userNames.erase(userNameMap);

    auto receiverMap = usersReceivers.find(user);
	if (receiverMap == usersReceivers.end())
	    return false;
    usersReceivers.erase(receiverMap);
	return true;
}

void GuildInformation::timeoutLoop() {
    std::cout << "Guild thread started for " << guildId << std::endl;
    while (running) {
        updateTimeout();
        std::this_thread::sleep_for(std::chrono::milliseconds(botInformation->checkTimeMilliseconds));
    }
    std::cout << "Guild thread exiting for " << guildId << std::endl;
}

void GuildInformation::updateTimeout() {
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
        
    auto now = std::chrono::steady_clock::now();
    std::chrono::steady_clock::time_point snapshot;
    {
        std::lock_guard<std::mutex> lock(lastUserSpokenTimeLock);
        snapshot = lastUserSpokenTime;
    }
    size_t promptSize = 0;
    {
        std::lock_guard<std::mutex> lock(userPromptLock);
        promptSize = userPrompt.size();
    }
    auto lastSpokenDuration = std::chrono::duration_cast<std::chrono::milliseconds>(now - snapshot).count();
    if (promptSize == 0) return;
    if (lastSpokenDuration < botInformation->characterSettings.emptySpaceTimeout)
        if (promptSize < botInformation->characterSettings.minBufferSize) 
            return;
        
    if (lastSpokenDuration < botInformation->characterSettings.minSpokenTimeout) {
        //std::cout << "Prevented premature response (talking over) at -> " << lastSpokenDuration << " milliseconds. Needed -> " << botInformation->characterSettings.minSpokenTimeout << " milliseconds.\n";
        return;
    }

    {
        std::lock_guard<std::mutex> lock(lastResponseLock);
        snapshot = lastResponse;
    }
	auto lastResponseDuration = std::chrono::duration_cast<std::chrono::milliseconds>(now - snapshot).count();
    if (lastResponseDuration < botInformation->characterSettings.minResponseTime) {
        //std::cout << "Prevented premature response at -> " << lastResponseDuration << " milliseconds. Needed -> " << MIN_TIME_BETWEEN_RESPONSES_MILLISECONDS << " milliseconds.\n";
        return;
    }
        
    bool expected = false;
    if (!isVoiceThreadRunning.compare_exchange_strong(expected, true))
        return;
    if (voiceThread.joinable())
        voiceThread.join();
    voiceThread = std::thread([this] {
        {
        std::lock_guard<std::mutex> lock(lastResponseLock);
        lastResponse = std::chrono::steady_clock::now();
        }
        this->sendPrompt();
        isVoiceThreadRunning.store(false);
    });
}

void GuildInformation::processNewSpeechToText(std::vector<int16_t>* buffer, const dpp::snowflake& user) {
    std::string stt = speechToText.getResponse(buffer, user);
    if (stt.empty()) return;
	std::cout << stt << std::endl;

    std::string name = "";
    auto userName = userNames.find(user);
    if (userName != userNames.end()) name = userName->second;
    std::string newPrompt;
    newPrompt.reserve(100);
    newPrompt.append(name).append(", \"").append(stt).append("\"\n");

    std::lock_guard<std::mutex> lock(userPromptLock);
    userPrompt.append(std::move(newPrompt));

    std::cout << "Added new object to list.\n";
}

std::vector<uint8_t> GuildInformation::upsampleAndConvert(const std::vector<int16_t>& input) {
    std::vector<uint8_t> output;
    output.reserve(input.size() * 4);

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
void GuildInformation::sendPrompt() {
    std::string result = "";
    {
    std::lock_guard<std::mutex> lock(userPromptLock);
    std::cout << "- NEW PROMPT WILL BE SENT -\n";
    std::cout << userPrompt << '\n';

    result = serviceHandler.openAiResponse(userPrompt, botInformation->characterSettings.characterPrompt);
    std::cout << "- RESULT -\n";
    std::cout << result << '\n';
    }

    std::vector<int16_t> audioResult = serviceHandler.elevenLabsTextToSpeech(result);
    //savePCMToFile(audioResult, "TMP.pcm");
    auto test = upsampleAndConvert(audioResult);
    streamPCMToVoiceChannel(test);
    userPrompt.clear();
}

void GuildInformation::streamPCMToVoiceChannel(std::vector<uint8_t>& pcm_data) {
    if (!connection || !connection->voiceclient) {
        std::cout << "Invalid voice connection.\n";
        return;
    }

    int remainder = pcm_data.size() % 4;
    if (remainder != 0) {
        std::cout << "Trimming PCM buffer for alignment.\n";
        for (int i = 0; i < remainder; i++) pcm_data.pop_back();
    }

    std::lock_guard<std::mutex> lock(voiceSendMutex);
    std::cout << "Sending audio raw..." << std::endl;
    connection->voiceclient->send_audio_raw(reinterpret_cast<uint16_t*>(pcm_data.data()), pcm_data.size());
}

void GuildInformation::savePCMToFile(const std::vector<int16_t>& audioData, const std::string& filename) {
    std::ofstream outFile(filename, std::ios::binary);
    if (!outFile) {
        std::cerr << "Failed to open file for writing: " << filename << std::endl;
        return;
    }

    outFile.write(reinterpret_cast<const char*>(audioData.data()), audioData.size() * sizeof(int16_t));
    outFile.close();
    std::cout << "Saved " << audioData.size() << " samples to " << filename << std::endl;
}