#include "speechToText.h"

std::string SpeechToText::getResponse(std::vector<int16_t>* buffer, const dpp::snowflake& user) {
	if (!leopard || buffer->empty()) return "";

	char* transcript = nullptr;
	int32_t numWords = 0;
	pv_word_t* words = nullptr;
	std::string result;

	pv_status_t status = pv_leopard_process(leopard, buffer->data(), buffer->size(), &transcript, &numWords, &words);
	if (status == PV_STATUS_SUCCESS && transcript) {
		result = transcript;
		pv_leopard_transcript_delete(transcript);
	} else {
		std::cerr << "Leopard processing failed with status: " << status << std::endl;
	}
	if (words) pv_leopard_words_delete(words);

	return result;
}
