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
	SpeechToText(const dpp::snowflake& guild, const char* leopard) : 
        guildId(guild), LEOPARD(leopard) { }

	std::string getResponse(std::vector<int16_t>* buffer, const dpp::snowflake& user);

private:
	const dpp::snowflake guildId;
    const char* LEOPARD;
};
