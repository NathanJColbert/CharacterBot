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

class ServiceHandler {
public:
	ServiceHandler(const std::string& openAi_key) : OPENAI_KEY(openAi_key) { }

	std::string openAiResponse(const std::string& prompt, const std::string& systemPrompt);

private:
	const std::string OPENAI_KEY;

	std::deque<std::pair<std::string, std::string>> conversation_history;

	size_t estimateTokens(const std::string& text) { return text.size() / 4; }
	void truncateTokenHistory(std::deque<std::pair<std::string, std::string>>& history);
};