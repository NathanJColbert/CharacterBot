#pragma once
#include <dpp/dpp.h>
#include <thread>
#include <atomic>
#include <iostream>
#include "guildInformation.h"
#include "menuControl.h"
#include "messageContextResponse.h"

const long long IGNORE_VOICE_TIME_MILLISECONDS = 0;

// -----------------------------------------------------------------------------
// Character bot object. Wraps a single cluster object as the main character bot
// -----------------------------------------------------------------------------
class CharacterBot : public dpp::cluster {
public:
	CharacterBot(const char* token, BotInformation* information, MessageContextSettings* messageSettings) :
		botInformation(information), messageContextSettings(messageSettings), dpp::cluster(token) { }

	// -----------------------------------------------------------------------------
	// Runs the bot. Manages the threads on its own. This does not return
	// until shut down by use of commands.
	// -----------------------------------------------------------------------------
	void run();

private:
    BotInformation* botInformation;
	MessageContextSettings* messageContextSettings;
	std::unordered_map<dpp::snowflake, std::shared_ptr<GuildInformation>> guilds;
	std::unordered_map<dpp::snowflake, std::shared_ptr<MessageContextResponse>> messageContextGuilds;

	std::atomic<bool> lockVoiceInput{false};

	std::vector<dpp::voiceconn*> disconnectAll();
	void joinVoice(const dpp::slashcommand_t& event);
	void leaveVoice(const dpp::slashcommand_t& event);
	bool tryGetGuildInformation(std::shared_ptr<GuildInformation>& guildInformation, const dpp::snowflake& guild);
	bool addUserInVoice(const dpp::snowflake& guild, const dpp::snowflake& user);
	bool removeUserInVoice(dpp::discord_client* shard, const dpp::snowflake& guild, const dpp::snowflake& user);
	bool removeBotInGuildVoice(const dpp::snowflake& guild);
	void handleVoiceStateUpdate(const dpp::voice_state_update_t& event);
	void handleIncomingMessage(const dpp::message_create_t& event);

	bool testConnectionsActive(const std::vector<dpp::voiceconn*>& connections, int& total);
	bool activeInGuild(const dpp::snowflake& guild);
	std::shared_ptr<MessageContextResponse> getMessageContext(const dpp::snowflake guildId);
};
