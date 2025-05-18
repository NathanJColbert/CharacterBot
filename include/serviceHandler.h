#pragma once
#include <string>
#include <deque>
#include <dpp/dpp.h>
#include <curl/curl.h>

static size_t writeCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
	size_t total_size = size * nmemb;
	output->append(reinterpret_cast<char*>(contents), total_size);
	return total_size;
}
static size_t writeCallbackBinary(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t totalSize = size * nmemb;
    auto* vec = static_cast<std::vector<char>*>(userp);
    vec->insert(vec->end(), static_cast<char*>(contents), static_cast<char*>(contents) + totalSize);
    return totalSize;
}

class ServiceHandler {
public:
	ServiceHandler(const std::string& openAiKey, const std::string& elevenLabsKey, const std::string& elevenLabsSpeechId) : 
		OPENAI_KEY(openAiKey), ELEVEN_LABS_KEY(elevenLabsKey), ELEVEN_LABS_SPEECH_ID(elevenLabsSpeechId)
	{ curl_global_init(CURL_GLOBAL_DEFAULT); }
	~ServiceHandler()
	{ curl_global_cleanup(); }

	std::string openAiResponse(const std::string& prompt, const std::string& systemPrompt);
	std::vector<int16_t> elevenLabsTextToSpeech(const std::string& text);

private:
	const std::string OPENAI_KEY;
	const std::string ELEVEN_LABS_KEY;
	const std::string ELEVEN_LABS_SPEECH_ID;

	std::deque<std::pair<std::string, std::string>> conversation_history;

	size_t estimateTokens(const std::string& text) { return text.size() / 4; }
	void truncateTokenHistory(std::deque<std::pair<std::string, std::string>>& history, const std::string& systemPrompt);
};