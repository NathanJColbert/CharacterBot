#include "audioReceiver.h"

void AudioReceiver::onVoiceReceive(const dpp::voice_receive_t& data) {
	if (data.audio_data.size() <= 0) return;

	if (pcm_buffer.size() < MAX_BUFFER_SIZE)
		convertToLittleEndianAndResample(data.audio_data, 96000, 16000);

	if (pcm_buffer.size() >= MAX_BUFFER_SIZE) {
		//save_audio_to_file("audio");
		pcm_buffer.clear();
	}
	lastReceived = std::chrono::steady_clock::now();
}

bool AudioReceiver::checkTimeout(std::vector<int16_t>& buffer) {
	if (pcm_buffer.size() <= 0) return false;

	auto now = std::chrono::steady_clock::now();
	auto duration_since_last_receive = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastReceived).count();

	std::cout << duration_since_last_receive << " : " << TIMEOUT_MILLISECONDS << std::endl;
	if (duration_since_last_receive > TIMEOUT_MILLISECONDS) {
		std::cout << pcm_buffer.size() << " : " << MIN_BUFFER_SIZE << std::endl;
		if (pcm_buffer.size() > MIN_BUFFER_SIZE) {
			buffer = std::vector<int16_t>(pcm_buffer);
			//save_audio_to_file("audio");
			pcm_buffer.clear();
			return true;
		}
		pcm_buffer.clear();
	}
	return false;
}

void AudioReceiver::save_audio_to_file(const std::string& fileName) {
	std::string fullName = fileName + userId.str() + ".pcm";
	std::ofstream file(fullName, std::ios::binary);

	if (!file.is_open()) {
		std::cerr << "Error opening file for writing!" << std::endl;
		return;
	}

	file.write(reinterpret_cast<const char*>(&pcm_buffer[0]), pcm_buffer.size() * sizeof(int16_t));
	file.close();

	std::cout << "Saved " << pcm_buffer.size() << " samples to " << fullName << std::endl;
}

void AudioReceiver::convertToLittleEndianAndResample(
	const std::vector<uint8_t>& pcm_data,
	size_t original_sample_rate,
	size_t target_sample_rate)
{
	if (pcm_data.size() % 2 != 0) {
		std::cerr << "Input PCM data has an odd size. Each int16_t requires 2 bytes." << std::endl;
		return;
	}
	float resample_ratio = static_cast<float>(original_sample_rate) / target_sample_rate;
	size_t sample_count = pcm_data.size() / 2;

	size_t estimated_output_samples = static_cast<size_t>(sample_count / resample_ratio);

	for (float i = 0.0; i < sample_count; i += resample_ratio) {
		size_t index = i * 2;
		if (index + 1 >= pcm_data.size()) break;

		int16_t sample = static_cast<int16_t>((pcm_data[index + 1] << 8) | pcm_data[index]);

		pcm_buffer.emplace_back(sample);
	}
}
