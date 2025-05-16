#include "ServiceHandler.h"

std::string ServiceHandler::openAiResponse(const std::string& prompt, const std::string& systemPrompt) {
    std::string url = "https://api.openai.com/v1/chat/completions";

	conversation_history.push_back({ "user",prompt });
	truncateTokenHistory(conversation_history);

	dpp::json prompt_json;
	prompt_json["model"] = "gpt-4";  // Use GPT-3 or another model as needed
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

	curl_global_init(CURL_GLOBAL_DEFAULT);
	curl = curl_easy_init();

	if (curl) {
		// Set up the cURL options
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl, CURLOPT_POST, 1L);

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
		else {
			//std::cout << "Response from OpenAI API: " << std::endl;
			//std::cout << read_buffer << std::endl;
		}

		// Clean up
		curl_easy_cleanup(curl);
		curl_slist_free_all(headers);
	}

	std::string result = "";
	try {
		auto response_json = dpp::json::parse(read_buffer);
		// Correctly access the 'message' and 'content' fields in the response
		result = response_json["choices"][0]["message"]["content"];
		//std::cout << "Generated text: " << result << std::endl;
	}
	catch (const dpp::json::exception& e) {
		std::cerr << "Error parsing JSON response: " << e.what() << std::endl;
	}

	curl_global_cleanup();

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
