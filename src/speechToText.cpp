#include "speechToText.h"

std::string SpeechToText::getResponse(std::vector<int16_t>* buffer, const dpp::snowflake& user) {
	const char* modelPath = "languageModel.pv";

	pv_leopard_t* leopard = nullptr;
	pv_status_t status = pv_leopard_init(LEOPARD, modelPath, true, false, &leopard);
	if (status != PV_STATUS_SUCCESS) {
		std::cerr << "Failed to initialize Leopard." << std::endl;
		return "";
	}
	char* transcript = nullptr;
	int32_t numWords = 0;
	pv_word_t* words = nullptr;
	status = pv_leopard_process(leopard, buffer->data(), buffer->size(), &transcript, &numWords, &words);
	if (status == PV_STATUS_SUCCESS) {
		std::cout << "Transcript:\n" << transcript << std::endl;
		pv_leopard_transcript_delete(transcript);
	}
	else {
		std::cout << status << std::endl;
		return "";
	}

	pv_leopard_delete(leopard);
	return "";
}
