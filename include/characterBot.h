#pragma once
#include <dpp/dpp.h>
#include "audioReceiver.h"
#include "speechToText.h"

class CharacterBot : public dpp::cluster {
public:
	CharacterBot(const char* token, const char* openAI, const char* leopard) :
		OPEN_AI(openAI), LEOPARD(leopard), dpp::cluster(token) { }

	void run();

private:
    const char* OPEN_AI;
    const char* LEOPARD;

	struct GuildInformation {
		GuildInformation(std::unordered_map<dpp::snowflake, AudioReceiver*>* recievers, SpeechToText* stt, dpp::voiceconn* voiceConn) :
			usersReceivers(recievers), speechToText(stt), connection(voiceConn) { }
		std::unordered_map<dpp::snowflake, AudioReceiver*>* usersReceivers;
		SpeechToText* speechToText;
		dpp::voiceconn* connection;
	};
	std::unordered_map<dpp::snowflake, GuildInformation*> guilds;

	void joinVoice(const dpp::slashcommand_t& event);
	void updateTimeout();
	bool stringExists(const std::string& origin, const std::string& check);
	bool tryResponse(const std::string& stt);
	void processNewSpeechToText(std::vector<int16_t>* buffer, GuildInformation* guildObject, const dpp::snowflake& guild, const dpp::snowflake& user);
	bool tryGetAudioReceiver(AudioReceiver*& receiver, const dpp::snowflake& guild, const dpp::snowflake& user);
	bool addUserInVoice(const dpp::snowflake& guild, const dpp::snowflake& user);
	bool removeUserInVoice(dpp::discord_client* shard, const dpp::snowflake& guild, const dpp::snowflake& user);
	bool removeBotInGuildVoice(const dpp::snowflake& guild);
	void handle_voice_state_update(const dpp::voice_state_update_t& event);

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
