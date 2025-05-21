#include "characterBot.h"

const bool RESET_GLOBAL_COMMANDS = false;

std::atomic<bool> bot_running{true};
void CharacterBot::run() {
	//on_log(dpp::utility::cout_logger());

	intents = dpp::i_default_intents | dpp::i_message_content;

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
		if (lockVoiceInput) return;
		// Check sum?
		if (data.audio_data.size() == 3840) {
			std::shared_ptr<GuildInformation> guildObj = NULL;
			if (tryGetGuildInformation(guildObj, data.voice_client->server_id)) {
				auto now = std::chrono::steady_clock::now();
				auto currentDuration = std::chrono::duration_cast<std::chrono::milliseconds>(now - guildObj->lastResponseTime()).count();
				if (currentDuration < IGNORE_VOICE_TIME_MILLISECONDS) return;
				guildObj->sendVoiceData(data);
			}
		}
		});

	on_voice_state_update([this](const dpp::voice_state_update_t& event) {
		handleVoiceStateUpdate(event);
		});

	on_message_create([this](const dpp::message_create_t& event) {
		handleIncomingMessage(event);
		});

	start(dpp::st_return);
	std::cout << "Bot started successfully!" << std::endl;
	std::thread input_thread([this]() {
		std::string input;
		while (bot_running) {
			std::getline(std::cin, input);
			if (menuCompareInput(input, {"quit", "q", "exit", "e"})) {
				std::cout << "Shutting down bot..." << std::endl;
				bot_running = false;
			}
			else if (menuCompareInput(input, {"count", "c", "servercount"})) {
				if (guilds.size() <= 0) {
					std::cout << "Zero active guilds." << std::endl;
				}
				else {
					std::cout << "Total guilds: " << guilds.size() << std::endl;
					for (auto i = guilds.begin(); i != guilds.end(); i++) {
						std::cout << "\tGuildId: [" << i->first << "] active users in voice: " << i->second->userCount() << std::endl;
					}
				}
			}
			else if (menuCompareInput(input, {"stop", "s", "stopinput"})) {
				lockVoiceInput.store(true);
				std::cout << "Stopped voice input." << std::endl;
			}
			else if (menuCompareInput(input, {"resume", "r", "resumeinput"})) {
				lockVoiceInput.store(false);
				std::cout << "Resumed voice input." << std::endl;
			}
		}
	});

	while (bot_running) {
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
	}

	auto connections = disconnectAll();
	int count = 0;
	while (testConnectionsActive(connections, count)) {
		std::cout << "[" << connections.size() << "] Waiting for connections to close." << std::endl;
		std::this_thread::sleep_for(std::chrono::milliseconds(200));
	}

	if (input_thread.joinable()) {
		input_thread.join();
	}
}

bool CharacterBot::testConnectionsActive(const std::vector<dpp::voiceconn*>& connections, int& total) {
	total = 0;
	bool result = false;
	for (size_t i = 0; i < connections.size(); i ++) {
		if (connections[i] && connections[i]->is_active()) {
			result = true;
			total++;
		}
	}
	return result;
}

std::vector<dpp::voiceconn*> CharacterBot::disconnectAll() {
	std::vector<dpp::voiceconn*> output;
	output.reserve(guilds.size());
	for (auto i = guilds.begin(); i != guilds.end(); i++) {
		auto conn = i->second->getConnection();
		if (!conn) {
			std::cout << "Voice connection is null." << std::endl;
			continue;
		}
		conn->disconnect();
		output.emplace_back(conn);
	}
	return output;
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
	dpp::guild_member member = dpp::find_guild_member(guild, user);
	if (member.get_user()->is_bot()) return false;
	if (!member.get_nickname().empty()) {
		userName = member.get_nickname();
	} else {
		auto userObj = dpp::find_user(user);
		if (userObj) userName = userObj->username;
	}

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
	dpp::discord_client* conn = event.from();
	dpp::voiceconn* connection = conn->get_voice(guildId);
	auto newGuild = std::make_shared<GuildInformation>(guildId, connection, botInformation);
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
	event.reply("Left channel successfully");
}

bool CharacterBot::activeInGuild(const dpp::snowflake& guild) {
	auto guildIterator = guilds.find(guild);
	if (guildIterator != guilds.end()) return true;
	return false;
}

std::shared_ptr<MessageContextResponse> CharacterBot::getMessageContext(const dpp::snowflake guildId) {
	auto i = messageContextGuilds.find(guildId);
	if (i == messageContextGuilds.end()) {
		auto newContext = std::make_shared<MessageContextResponse>(messageContextSettings);
		messageContextGuilds.insert(std::pair(guildId, newContext));
		std::cout << "Created new message context for guild: " << guildId << std::endl;
		return newContext;
	}
	return i->second;
}

void CharacterBot::handleIncomingMessage(const dpp::message_create_t& event) {
	if (event.msg.author.is_bot()) return;
	//std::cout << "Message [" << event.msg.author.username << "]: " << event.msg.content << std::endl;
	dpp::message response;
	if (getMessageContext(event.msg.guild_id)->tryGetResponse(event, response)) {
		this->message_create(response);
	}
}
