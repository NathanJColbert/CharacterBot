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
	std::string result;
	status = pv_leopard_process(leopard, buffer->data(), buffer->size(), &transcript, &numWords, &words);
	if (status == PV_STATUS_SUCCESS && transcript != nullptr) {
        result = transcript;
        pv_leopard_transcript_delete(transcript);
    } else {
        std::cerr << "Leopard processing failed with status: " << status << std::endl;
    }
    if (words) pv_leopard_words_delete(words);

    pv_leopard_delete(leopard);
    return result;
}
