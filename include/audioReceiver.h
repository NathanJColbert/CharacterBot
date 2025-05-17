#pragma once
#include <dpp/dpp.h>
#include <iostream>

const size_t MIN_BUFFER_SIZE = 10000;
const size_t MAX_BUFFER_SIZE = 3200000;
const long long TIMEOUT_MILLISECONDS = 700;

class AudioReceiver {
public:
	AudioReceiver(const dpp::snowflake& user) : lastReceived(std::chrono::steady_clock::now()), userId(user) {
		pcm_buffer.reserve(MAX_BUFFER_SIZE);
	}
	~AudioReceiver() {
		std::cout << "Destroyed Reciever" << std::endl;
	}

	void onVoiceReceive(const dpp::voice_receive_t& data);
	bool checkTimeout(std::vector<int16_t>& buffer);
	void resetBuffer() { pcm_buffer.clear(); }

private:
	const dpp::snowflake userId;
	std::vector<int16_t> pcm_buffer;
	std::chrono::steady_clock::time_point lastReceived;

	void save_audio_to_file(const std::string& fileName);

	void convertToLittleEndianAndResample(const std::vector<uint8_t>& pcm_data,
		size_t original_sample_rate,
		size_t target_sample_rate);
};
