#pragma once
#include <dpp/dpp.h>
#include "guildInformation.h"

class CharacterBot : public dpp::cluster {
public:
	CharacterBot(const char* token, const char* openAI, const char* leopard, const char* elevenLabs) :
		OPEN_AI(openAI), LEOPARD(leopard), ELEVEN_LABS(elevenLabs), dpp::cluster(token) { }

	void run();

private:
    const char* OPEN_AI;
    const char* LEOPARD;
    const char* ELEVEN_LABS;
	std::unordered_map<dpp::snowflake, std::shared_ptr<GuildInformation>> guilds;

	void joinVoice(const dpp::slashcommand_t& event);
	void leaveVoice(const dpp::slashcommand_t& event);
	bool tryGetGuildInformation(std::shared_ptr<GuildInformation>& guildInformation, const dpp::snowflake& guild);
	bool addUserInVoice(const dpp::snowflake& guild, const dpp::snowflake& user);
	bool removeUserInVoice(dpp::discord_client* shard, const dpp::snowflake& guild, const dpp::snowflake& user);
	bool removeBotInGuildVoice(const dpp::snowflake& guild);
	void handleVoiceStateUpdate(const dpp::voice_state_update_t& event);

	bool activeInGuild(const dpp::snowflake& guild);
};
