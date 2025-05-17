#include "characterBot.h"

void CharacterBot::run() {
	on_log(dpp::utility::cout_logger());

	on_slashcommand([this](const dpp::slashcommand_t& event) {
		if (event.command.get_command_name() == "join") {
			joinVoice(event);
		}
		if (event.command.get_command_name() == "test") {
            leaveVoice(event);
		}
		});

	on_ready([this](const dpp::ready_t& event) {
		if (dpp::run_once<struct register_bot_commands>()) {
			global_command_create(dpp::slashcommand("join", "Joins channel", me.id));
			global_command_create(dpp::slashcommand("test", "A random test setup", me.id));
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

bool CharacterBot::stringExists(const std::string& origin, const std::string& check) {
	if (origin.size() < check.size()) return false;
	std::string originLower = origin;
	std::string checkLower = check;
	std::transform(originLower.begin(), originLower.end(), originLower.begin(), ::tolower);
	std::transform(checkLower.begin(), checkLower.end(), checkLower.begin(), ::tolower);
	return originLower.find(checkLower) != std::string::npos;
}

bool CharacterBot::tryResponse(const std::string& stt) {
	if (stt.size() < 10) return false;
	if (stringExists(stt, "paul")) return true;
	int randomNum = rand() % 101;
	return randomNum < 30;
}

void CharacterBot::processNewSpeechToText(std::vector<int16_t>* buffer, GuildInformation* guildObject, const dpp::snowflake& guild, const dpp::snowflake& user) {
	/*
	std::string stt = guildObject->speechToText->getResponse(buffer, user);
	std::cout << stt << std::endl;
	return;
	if (!tryResponse(stt)) { std::cout << "\tRefused the response." << std::endl; return; }

	std::string response = "";//handler->openAiResponse(stt, "You are a friend who is sarcastic to a fault. You use slang. You always seem to incorporate a dad joke into the mix.");
	std::cout << "OpenAI responded with: " << response << std::endl;
	std::cout << "Guild object finished processing response -> " << response << std::endl;

	std::string outputFileName = "";//handler->elevenLabsTextToSpeech(response, "tmp.mp3");
	//streamMP3ToVoiceChannel(guildObject->connection, outputFileName);
	//streamPCMToVoiceChannel(guildObject->connection, outputFileName);

	for (auto i = guildObject->usersReceivers->begin(); i != guildObject->usersReceivers->end(); i++) {
		i->second->resetBuffer();
	}
	std::cout << "Purged audio recievers for guild..." << std::endl;*/
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

std::vector<uint16_t> CharacterBot::resampleTo48kHz(const std::vector<uint16_t>& raw_pcm_data_16k) {
	size_t num_samples_16k = raw_pcm_data_16k.size();

	size_t num_samples_48k = num_samples_16k * 6;
	std::vector<uint16_t> pcm_data_48k(num_samples_48k);

	size_t i = 0;
	size_t j = 0;

	while (i < num_samples_16k - 1) {
		uint16_t current_sample = raw_pcm_data_16k[i];
		pcm_data_48k[j++] = current_sample;
		pcm_data_48k[j++] = current_sample;
		pcm_data_48k[j++] = current_sample;
		pcm_data_48k[j++] = current_sample;
		pcm_data_48k[j++] = current_sample;
		pcm_data_48k[j++] = current_sample;

		i++;
	}

	pcm_data_48k[j++] = raw_pcm_data_16k[i];

	return pcm_data_48k;
}

void CharacterBot::streamPCMToVoiceChannel(dpp::voiceconn* vc, const std::string& fileName) {
	std::ifstream pcmFile(fileName, std::ios::binary);
	if (!pcmFile.is_open()) {
		std::cerr << "Error: Could not open PCM file " << fileName << std::endl;
		return;
	}
	pcmFile.seekg(0, std::ios::end);
	size_t fileSize = pcmFile.tellg();
	pcmFile.seekg(0, std::ios::beg);
	size_t numSamples = fileSize;
	std::vector<uint16_t> pcmData(numSamples);

	pcmFile.read(reinterpret_cast<char*>(pcmData.data()), fileSize);

	if (pcmFile.gcount() != fileSize) {
		std::cerr << "Error: Failed to read the entire PCM file" << std::endl;
		pcmFile.close();
		return;
	}

	pcmFile.close();

	std::vector<uint16_t> tmp = resampleTo48kHz(pcmData);
	streamPCMToVoiceChannel(vc, tmp);
}

void CharacterBot::streamPCMToVoiceChannel(dpp::voiceconn* vc, std::vector<uint16_t>& pcm_data) {
	if (!vc || !vc->voiceclient) return;
	vc->voiceclient->send_audio_raw(pcm_data.data(), pcm_data.size());
}

/*
bool CharacterBot::convertMP3ToRawPCMFFMPEG(std::vector<uint16_t>& pcm_data, const std::string& fileName) {
	// Initialize FFmpeg libraries
	avformat_network_init();

	// Open input file
	AVFormatContext* formatContext = nullptr;
	if (avformat_open_input(&formatContext, fileName.c_str(), nullptr, nullptr) < 0) {
		std::cerr << "Error opening input file." << std::endl;
		return false;
	}

	// Retrieve stream information
	if (avformat_find_stream_info(formatContext, nullptr) < 0) {
		std::cerr << "Error finding stream information." << std::endl;
		return false;
	}

	// Find the audio stream
	int audioStreamIndex = -1;
	for (int i = 0; i < formatContext->nb_streams; i++) {
		if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
			audioStreamIndex = i;
			break;
		}
	}

	if (audioStreamIndex == -1) {
		std::cerr << "Audio stream not found." << std::endl;
		return false;
	}

	// Find the decoder for the audio stream
	AVCodecParameters* codecParameters = formatContext->streams[audioStreamIndex]->codecpar;
	const AVCodec* codec = avcodec_find_decoder(codecParameters->codec_id);
	if (!codec) {
		std::cerr << "Codec not found." << std::endl;
		return false;
	}

	// Open the codec
	AVCodecContext* codecContext = avcodec_alloc_context3(codec);
	if (!codecContext) {
		std::cerr << "Could not allocate codec context." << std::endl;
		return false;
	}

	if (avcodec_parameters_to_context(codecContext, codecParameters) < 0) {
		std::cerr << "Could not copy codec parameters." << std::endl;
		return false;
	}

	if (avcodec_open2(codecContext, codec, nullptr) < 0) {
		std::cerr << "Could not open codec." << std::endl;
		return false;
	}

	// Initialize the resampler to convert to 16-bit stereo 48kHz
	SwrContext* swrContext = swr_alloc();
	if (!swrContext) {
		std::cerr << "Could not allocate resampler." << std::endl;
		return false;
	}

	AVChannelLayout outChannelLayout;
	av_channel_layout_default(&outChannelLayout, AV_CH_LAYOUT_STEREO);
	// Set the resampler options
	int ret = swr_alloc_set_opts2(&swrContext,
		&outChannelLayout,         // Output channel layout (Stereo)
		AV_SAMPLE_FMT_S16,        // Output sample format (16-bit signed)
		48000,                    // Output sample rate (48kHz)
		&codecContext->ch_layout,  // Input channel layout
		codecContext->sample_fmt, // Input sample format
		codecContext->sample_rate,// Input sample rate
		0, 0);                    // Flags

	if (ret < 0) {
		std::cerr << "Could not set resampler options." << std::endl;
		return false;
	}

	if (swr_init(swrContext) < 0) {
		std::cerr << "Could not initialize resampler." << std::endl;
		return false;
	}

	// Read and decode the audio frames
	AVPacket packet;
	AVFrame* frame = av_frame_alloc();
	while (av_read_frame(formatContext, &packet) >= 0) {
		if (packet.stream_index == audioStreamIndex) {
			if (avcodec_send_packet(codecContext, &packet) < 0) {
				av_packet_unref(&packet);
				continue;
			}

			while (avcodec_receive_frame(codecContext, frame) >= 0) {
				// Calculate the output buffer size for the resampled data
				int outputBufferSize = av_samples_get_buffer_size(nullptr, 2, frame->nb_samples, AV_SAMPLE_FMT_S16, 0);
				if (outputBufferSize < 0) {
					std::cerr << "Failed to get output buffer size." << std::endl;
					continue;
				}

				// Allocate memory for the output buffer
				uint8_t* outputBuffer = (uint8_t*)av_malloc(outputBufferSize);
				if (!outputBuffer) {
					std::cerr << "Could not allocate memory for output buffer." << std::endl;
					continue;
				}

				int resampledFrameCount = swr_convert(swrContext, &outputBuffer, frame->nb_samples, (const uint8_t**)frame->data, frame->nb_samples);
				if (resampledFrameCount < 0) {
					std::cerr << "Resampling failed." << std::endl;
					av_free(outputBuffer);
					continue;
				}

				// Process PCM data and add to pcm_data
				for (int i = 0; i < resampledFrameCount * 2; ++i) { // Stereo (2 channels)
					pcm_data.push_back(((int16_t*)outputBuffer)[i]);
				}

				// Clean up
				av_free(outputBuffer);  // Free memory allocated by FFmpeg
			}
		}

		av_packet_unref(&packet);
	}

	// Cleanup
	av_frame_free(&frame);
	swr_free(&swrContext);
	avcodec_free_context(&codecContext);
	avformat_close_input(&formatContext);

	return true;
}

bool CharacterBot::convertMP3ToRawPCM(std::vector<uint16_t>& pcm_data, const std::string& fileName) {
	mpg123_handle* mh = nullptr;
	unsigned char* buffer = nullptr;
	size_t buffer_size = 8192;
	size_t done;
	int err;
	int channels, encoding;
	long rate;

	// Initialize mpg123 library
	if (mpg123_init() != MPG123_OK) {
		std::cerr << "mpg123 initialization failed!" << std::endl;
		return false;
	}

	// Open the MP3 file
	mh = mpg123_new(nullptr, &err);
	if (mh == nullptr) {
		std::cerr << "mpg123 handle creation failed!" << std::endl;
		mpg123_exit();
		return false;
	}

	// Open the MP3 file
	if (mpg123_open(mh, fileName.c_str()) != MPG123_OK) {
		std::cerr << "Unable to open MP3 file!" << std::endl;
		mpg123_delete(mh);
		mpg123_exit();
		return false;
	}

	// Get the MP3 file info (this helps to check the format)
	if (mpg123_getformat(mh, &rate, &channels, &encoding) != MPG123_OK) {
		std::cerr << "Unable to read MP3 format!" << std::endl;
		mpg123_delete(mh);
		mpg123_exit();
		return false;
	}

	std::cout << "Rate: " << rate << ", Channels: " << channels << ", Encoding: " << encoding << std::endl;

	// Set the output format to 16-bit unsigned PCM stereo at 48 kHz
	if (mpg123_format(mh, 48000, 2, MPG123_ENC_UNSIGNED_16) != MPG123_OK) {
		std::cerr << "Unable to set output format!" << std::endl;
		mpg123_delete(mh);
		mpg123_exit();
		return false;
	}

	// Allocate buffer for decoding
	buffer = new unsigned char[buffer_size];

	// Read until the entire MP3 file is processed
	while (true) {
		int result = mpg123_read(mh, buffer, buffer_size, &done);

		if (result == MPG123_ERR) {
			std::cerr << "Error while reading MP3 data: " << mpg123_strerror(mh) << std::endl;
			break;
		}
		else if (result == MPG123_DONE || result == MPG123_OK) {
			// Process the decoded PCM data
			//std::cout << "Read " << done << " bytes from MP3 file." << std::endl;

			// Handle the decoded PCM samples
			for (size_t i = 0; i < done / 2; i++) {  // We process two bytes per sample (16-bit)
				uint16_t pcm_sample = *(uint16_t*)&buffer[i * 2];

				// If stereo, duplicate the mono sample to both channels
				pcm_data.push_back(pcm_sample);  // Left channel
				pcm_data.push_back(pcm_sample);  // Right channel
			}

			if (result == MPG123_DONE) {
				std::cout << "End of MP3 file reached." << std::endl;
				break;
			}
		}
		else {
			std::cerr << "Unexpected error while reading: " << mpg123_strerror(mh) << std::endl;
			break;
		}

		// Example of seeking to the beginning of the file after some processing
		if (done < buffer_size) {
			std::cout << "Seeking back to the start of the file..." << std::endl;
			mpg123_seek(mh, 0, SEEK_SET);  // Reset to the beginning
		}
	}

	// Cleanup
	delete[] buffer;
	mpg123_close(mh);
	mpg123_delete(mh);
	mpg123_exit();

	return true;
}

void CharacterBot::streamMP3ToVoiceChannel(dpp::voiceconn* vc, const std::string& fileName) {
	std::vector<uint16_t> pcm_data;
	convertMP3ToRawPCM(pcm_data, fileName);
	streamPCMToVoiceChannel(vc, pcm_data);
}
*/


bool CharacterBot::activeInGuild(const dpp::snowflake& guild) {
	auto guildIterator = guilds.find(guild);
	if (guildIterator != guilds.end()) return true;
	return false;
}