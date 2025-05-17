#pragma once
#include <dpp/dpp.h>
#include <iostream>

struct AudioReceiverSettings {
	AudioReceiverSettings() = default;
	AudioReceiverSettings(size_t minBuffer, size_t maxBuffer, long long timeout) :
	minBufferSize(minBuffer), maxBufferSize(maxBuffer), timoutMilliseconds(timeout)
	{ }
	size_t minBufferSize;
	size_t maxBufferSize;
	long long timoutMilliseconds;
};

class AudioReceiver {
public:
	AudioReceiver(const dpp::snowflake& user, AudioReceiverSettings* audioReceiversettings) : 
	lastReceived(std::chrono::steady_clock::now()), 
	userId(user),
	settings(audioReceiversettings) {
		pcm_buffer.reserve(settings->maxBufferSize);
	}
	~AudioReceiver() {
		std::cout << "Destroyed Reciever" << std::endl;
	}

	void onVoiceReceive(const dpp::voice_receive_t& data);
	bool checkTimeout(std::vector<int16_t>& buffer);
	void resetBuffer() { pcm_buffer.clear(); }

private:
	const dpp::snowflake userId;
	AudioReceiverSettings* settings;
	std::vector<int16_t> pcm_buffer;
	std::chrono::steady_clock::time_point lastReceived;

	void save_audio_to_file(const std::string& fileName);

	void convertToLittleEndianAndResample(const std::vector<uint8_t>& pcm_data,
		size_t original_sample_rate,
		size_t target_sample_rate);
};
