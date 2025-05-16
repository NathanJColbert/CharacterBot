#pragma once
#include <dpp/dpp.h>
#include "guildInformation.h"

class CharacterBot : public dpp::cluster {
public:
	CharacterBot(const char* token, const char* openAI, const char* leopard) :
		OPEN_AI(openAI), LEOPARD(leopard), dpp::cluster(token) { }

	void run();

private:
    const char* OPEN_AI;
    const char* LEOPARD;
	std::unordered_map<dpp::snowflake, std::shared_ptr<GuildInformation>> guilds;

	void joinVoice(const dpp::slashcommand_t& event);
	bool stringExists(const std::string& origin, const std::string& check);
	bool tryResponse(const std::string& stt);
	void processNewSpeechToText(std::vector<int16_t>* buffer, GuildInformation* guildObject, const dpp::snowflake& guild, const dpp::snowflake& user);
	bool tryGetGuildInformation(std::shared_ptr<GuildInformation>& guildInformation, const dpp::snowflake& guild);
	bool addUserInVoice(const dpp::snowflake& guild, const dpp::snowflake& user);
	bool removeUserInVoice(dpp::discord_client* shard, const dpp::snowflake& guild, const dpp::snowflake& user);
	bool removeBotInGuildVoice(const dpp::snowflake& guild);
	void handleVoiceStateUpdate(const dpp::voice_state_update_t& event);

	std::vector<uint16_t> resampleTo48kHz(const std::vector<uint16_t>& pcm_data_16k);

	/*
	bool convertMP3ToRawPCMFFMPEG(std::vector<uint16_t>& pcm_data, const std::string& fileName);
	bool convertMP3ToRawPCM(std::vector<uint16_t>& pcm_data, const std::string& fileName);
	void streamMP3ToVoiceChannel(dpp::voiceconn* vc, const std::string& fileName);
	*/

	void streamPCMToVoiceChannel(dpp::voiceconn* vc, const std::string& fileName);
	void streamPCMToVoiceChannel(dpp::voiceconn* vc, std::vector<uint16_t>& pcm_data);

	bool activeInGuild(const dpp::snowflake& guild);
};
