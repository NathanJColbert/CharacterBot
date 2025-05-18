#pragma once
#include <string>
#include <vector>
#include <dpp/dpp.h>
extern "C" {
	// Idk man...
	#define PV_API __declspec(dllimport)
	#include "pv_leopard.h"
}

class SpeechToText {
public:
	SpeechToText(const dpp::snowflake& guild, const char* leopardKey) : 
        guildId(guild), LEOPARD(leopardKey) {
		const char* modelPath = "languageModel.pv";
		pv_status_t status = pv_leopard_init(LEOPARD, modelPath, true, false, &leopard);
		if (status != PV_STATUS_SUCCESS || leopard == nullptr) {
			std::cerr << "Failed to initialize Leopard. Status: " << status << std::endl;
			leopard = nullptr;
		}
	}
	~SpeechToText() {
		if (leopard) pv_leopard_delete(leopard);
	}

	std::string getResponse(std::vector<int16_t>* buffer, const dpp::snowflake& user);

private:
	const dpp::snowflake guildId;
    const char* LEOPARD;
	pv_leopard_t* leopard = nullptr;
};
