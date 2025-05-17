#include "ServiceHandler.h"

std::string ServiceHandler::openAiResponse(const std::string& prompt, const std::string& systemPrompt) {
    std::string url = "https://api.openai.com/v1/chat/completions";

	conversation_history.push_back({ "user",prompt });
	truncateTokenHistory(conversation_history);

	dpp::json prompt_json;
	prompt_json["model"] = "gpt-4";
	prompt_json["max_tokens"] = 100;

	dpp::json messages = dpp::json::array();
	messages.push_back({ {"role", "system"}, {"content", systemPrompt} });

	for (const auto& message : conversation_history) {
		messages.push_back({ {"role", message.first}, {"content", message.second} });
	}
	prompt_json["messages"] = messages;

	std::string json_str = prompt_json.dump();

	CURL* curl;
	CURLcode res;
	std::string read_buffer;

	curl = curl_easy_init();

	if (curl) {
		// Set up the cURL options
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl, CURLOPT_POST, 1L);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
		curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);

		// Set the API key in the header
		struct curl_slist* headers = NULL;
		headers = curl_slist_append(headers, "Content-Type: application/json");
		headers = curl_slist_append(headers, ("Authorization: Bearer " + OPENAI_KEY).c_str());
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

		// Set the JSON data to send
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_str.c_str());

		// Set up a callback to capture the response
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &read_buffer);

		// Execute the API call
		res = curl_easy_perform(curl);

		// Check if the request was successful
		if (res != CURLE_OK) {
			std::cerr << "cURL request failed: " << curl_easy_strerror(res) << std::endl;
		}

		long http_code = 0;
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
		if (http_code != 200) {
    		std::cerr << "OpenAI API responded with HTTP code " << http_code << std::endl;
    		return "[Error: API request failed]";
		}

		// Clean up
		curl_easy_cleanup(curl);
		curl_slist_free_all(headers);
	}

	std::string result = "";
	try {
    	auto response_json = dpp::json::parse(read_buffer);
    	if (response_json.contains("choices") && response_json["choices"].is_array() &&
        	!response_json["choices"].empty() &&
        	response_json["choices"][0].contains("message") &&
        	response_json["choices"][0]["message"].contains("content")) {
        	result = response_json["choices"][0]["message"]["content"];
    	} else {
        	std::cerr << "Malformed API response: " << read_buffer << '\n';
        	result = "[Error: Malformed API response]";
    	}
	}
	catch (const dpp::json::exception& e) {
    	std::cerr << "JSON parsing error: " << e.what() << std::endl;
    	result = "[Error: JSON parsing failed]";
	}

	conversation_history.push_back({ "assistant", result });

    return result;
}

void ServiceHandler::truncateTokenHistory(std::deque<std::pair<std::string, std::string>>& history) {
	size_t total_tokens = 0;
	for (const auto& message : history) {
		total_tokens += estimateTokens(message.second);
	}

	const size_t MAX_TOKENS = 2000;
	while (total_tokens > MAX_TOKENS) {
		history.pop_front();  // Remove the oldest message
		total_tokens = 0;
		for (const auto& message : history) {
			total_tokens += estimateTokens(message.second);
		}
	}
}

std::vector<int16_t> ServiceHandler::elevenLabsTextToSpeech(const std::string& text) {
	// outputs signed little endian 16000 Hz
	CURL* curl;
	CURLcode res;
	std::vector<char> response_data;

	curl = curl_easy_init();

	if (curl) {
		std::string url = "https://api.elevenlabs.io/v1/text-to-speech/" + ELEVEN_LABS_SPEECH_ID + "?output_format=pcm_24000";

		struct curl_slist* headers = NULL;
		headers = curl_slist_append(headers, "Content-Type: application/json");
		headers = curl_slist_append(headers, ("xi-api-key: " + ELEVEN_LABS_KEY).c_str());

		dpp::json payload = {
			{"text", text},
			{"model_id", "eleven_flash_v2_5"}
		};

		std::string json_payload = payload.dump();

		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_payload.c_str());
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallbackBinary);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);
		res = curl_easy_perform(curl);

		if (res != CURLE_OK) {
			std::cerr << "cURL failed: " << curl_easy_strerror(res) << std::endl;
		}

		curl_easy_cleanup(curl);
		curl_slist_free_all(headers);
	}

	if (response_data.size() < 200) {
		std::string possible_error(response_data.begin(), response_data.end());
		std::cout << "Response (as string):\n" << possible_error << "\n";
	}

	// Convert binary char data to vector<int16_t>
	std::vector<int16_t> pcm_data;
	size_t sample_count = response_data.size() / sizeof(int16_t);
	pcm_data.resize(sample_count);

	std::memcpy(pcm_data.data(), response_data.data(), response_data.size());

	std::cout << "Extracted " << pcm_data.size() << " PCM samples.\n";
	return pcm_data;
}