#include "characterBot.h"

const bool RESET_GLOBAL_COMMANDS = false;

void CharacterBot::run() {
	on_log(dpp::utility::cout_logger());

	on_slashcommand([this](const dpp::slashcommand_t& event) {
		if (event.command.get_command_name() == "join") {
			joinVoice(event);
		}
		if (event.command.get_command_name() == "leave") {
            leaveVoice(event);
		}
		});

	on_ready([this](const dpp::ready_t& event) {
		if (RESET_GLOBAL_COMMANDS) {
			global_bulk_command_delete([](const dpp::confirmation_callback_t& callback) {
    		if (callback.is_error()) {
        		std::cerr << "Failed to delete all global commands: " << callback.get_error().message << std::endl;
    		} else {
        		std::cout << "All global commands deleted successfully." << std::endl;
    		}
		});
		}
		if (dpp::run_once<struct register_bot_commands>()) {
			global_command_create(dpp::slashcommand("join", "Joins the active channel", me.id));
			global_command_create(dpp::slashcommand("leave", "Leaves the active channel", me.id));
		}
		});


	on_voice_receive([this](const dpp::voice_receive_t& data) {
		// Check sum?
		if (data.audio_data.size() == 3840) {
			std::shared_ptr<GuildInformation> guildObj = NULL;
			if (tryGetGuildInformation(guildObj, data.voice_client->server_id))
				guildObj->sendVoiceData(data);
		}
		});

	on_voice_state_update([this](const dpp::voice_state_update_t& event) {
		handleVoiceStateUpdate(event);
		});

	start(dpp::st_wait);
	/*
	while (true) {
		std::this_thread::sleep_for(std::chrono::milliseconds(400));
	}*/
}

bool CharacterBot::tryGetGuildInformation(std::shared_ptr<GuildInformation>& guildInformation, const dpp::snowflake& guild) {
	auto it = guilds.find(guild);
	if (it == guilds.end())
		return false;
	guildInformation = it->second;
	return true;
}

bool CharacterBot::addUserInVoice(const dpp::snowflake& guild, const dpp::snowflake& user) {
	std::shared_ptr<GuildInformation> guildObj = NULL;
	if (!tryGetGuildInformation(guildObj, guild)) {
		std::cout << "Having issues finding a guild for the client." << std::endl;
		return false;
	}

	std::string userName = "";
	auto userObj = dpp::find_user(user);
	if (userObj != NULL) userName = userObj->username;
	if (!guildObj->addUser(user, userName)) {
		std::cout << "The user is already in the snowflake map! Skipping..." << std::endl;
		return false;
	}
	return true;
}

bool CharacterBot::removeUserInVoice(dpp::discord_client* shard, const dpp::snowflake& guild, const dpp::snowflake& user) {
	std::shared_ptr<GuildInformation> guildObj = NULL;
	if (!tryGetGuildInformation(guildObj, guild)) {
		std::cout << "Having issues finding a guild for a user disconnect. This indicates a memory leak." << std::endl;
		return false;
	}

	if (!guildObj->removeUser(user)) {
		std::cout << "User: " << user << " could not be found in guild " << guild << ".\n";
		return false;
	}
	std::cout << "Removed User " << user << std::endl;
	if (guildObj->userCount() <= 1) {
		shard->disconnect_voice(guild);
		removeBotInGuildVoice(guild);
	}
	return true;
}

bool CharacterBot::removeBotInGuildVoice(const dpp::snowflake& guild) {
	auto guildIterator = guilds.find(guild);
	if (guildIterator == guilds.end()) {
		std::cout << "Guild could not be found " << guild << " ignoring..." << std::endl;
		return false;
	}
	guilds.erase(guildIterator);
	return true;
}

void CharacterBot::handleVoiceStateUpdate(const dpp::voice_state_update_t& event) {
	if (!activeInGuild(event.state.guild_id)) return;
	if (event.state.channel_id != dpp::snowflake()) {
		std::cout << "User " << event.state.user_id << " joined voice channel." << std::endl;
		addUserInVoice(event.state.guild_id, event.state.user_id);
	}
	else {
		std::cout << "User " << event.state.user_id << " left voice channel." << std::endl;
		removeUserInVoice(event.from(), event.state.guild_id, event.state.user_id);
	}
}

void CharacterBot::joinVoice(const dpp::slashcommand_t& event) {
	dpp::guild* guild = dpp::find_guild(event.command.guild_id);
    if (!guild) {
        event.reply("Guild not found!");
        return;
    }
	auto guildIterator = guilds.find(event.command.guild_id);
	if (guildIterator != guilds.end()) {
		std::cout << "Having issues when joining a guild... This indicates a memory leak" << std::endl;
		return;
	}
    if (!guild->connect_member_voice(*event.owner, event.command.get_issuing_user().id)) {
        event.reply("You don't seem to be in a voice channel!");
        return;
    }
	event.reply("Joined your channel!");

	dpp::snowflake guildId = event.command.guild_id;
	dpp::discord_client* connection = event.from();
	auto newGuild = std::make_shared<GuildInformation>(guildId, connection, OPEN_AI, LEOPARD, ELEVEN_LABS);
	guilds.insert(std::pair(guildId, newGuild));

	// Update all users already in the channel
	dpp::voicestate voice_state = (guild->voice_members).find(event.command.get_issuing_user().id)->second;
	for (const auto& voice_state_entry : guild->voice_members) {
		// Check if the user is in the same voice channel as the issuing user
		if (voice_state_entry.second.channel_id == voice_state.channel_id) {
			addUserInVoice(event.command.guild_id, voice_state_entry.first);
		}
	}
}

void CharacterBot::leaveVoice(const dpp::slashcommand_t& event) {
	dpp::guild* guild = dpp::find_guild(event.command.guild_id);
    if (!guild) {
        event.reply("Guild not found!");
        return;
    }
	event.from()->disconnect_voice(guild->id);
	removeBotInGuildVoice(guild->id);
}

bool CharacterBot::activeInGuild(const dpp::snowflake& guild) {
	auto guildIterator = guilds.find(guild);
	if (guildIterator != guilds.end()) return true;
	return false;
}
