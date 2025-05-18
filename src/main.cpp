#include <string>
#include <iostream>
#include <cstdlib>
#include "characterBot.h"
#include "ServiceHandler.h"
#include "ARGControl.h"

// MEH!
//const char* CHARACTER_PROMPT = "You are a helpful programmer. You are there to guide your friends on their programming homework. Respond as if you are an individual in a group. Do not speak in the third person. Keep your response short.";
//const char* ELEVEN_LABS_SPEECH_ID = "JBFqnCBsd6RMkjVDRZzb";

const char* CHARACTER_PROMPT = "Your name is Palamedes. You are a great and powerful fantasy knight. You are in a discord call with your friends. Respond as if you are an individual in a group. Do not speak in the third person. Keep your response short";
const char* ELEVEN_LABS_SPEECH_ID = "N2lVS1w4EtoT3dr4eOWO";

//const char* CHARACTER_PROMPT = "Your name is Potion seller. You are a grummpy old man who refuses to sell potions to anyone. This is because you believe your potions are too strong for anyone. Respond as if you are an individual in a group. Do not speak in the third person. Keep your response short";
//const char* ELEVEN_LABS_SPEECH_ID = "zQtrcVyDViAdtX0qcTTG";

//const char* CHARACTER_PROMPT = "Your name is Jarvis. You are a brilliant AI. Respond as if you are an individual in a group. Do not speak in the third person. Keep your response short";
//const char* ELEVEN_LABS_SPEECH_ID = "wDsJlOXPqcvIUKdLXjDs";

const long long CHECK_TIME_MILLISECONDS = 200;

const long long MIN_TIME_BETWEEN_RESPONSES_MILLISECONDS = 10000;
const size_t BUFFER_SIZE_TRIGGER = 50;
const long long MIN_TIMEOUT_RESPONSE = 900;
const long long EMPTY_SPACE_TIMEOUT = 10000;

const size_t MIN_BUFFER_SIZE = 8000;
const size_t MAX_BUFFER_SIZE = 3200000;
const long long TIMEOUT_MILLISECONDS = 700;

int main(int argc, char* argv[]) {
    // Protect your keys!!!
    bool environmentFail = false;
    const char* botToken = std::getenv("BOT_TOKEN");
    if (botToken == NULL) {
        std::cout << "Bot token does not exist in environment. export BOT_TOKEN=\"TOKEN\"\n";
        environmentFail = true;
    }
    const char* openAIToken = std::getenv("OPEN_AI_TOKEN");
    if (openAIToken == NULL) {
        std::cout << "Open AI token does not exist in environment. export OPEN_AI_TOKEN=\"TOKEN\"\n";
        environmentFail = true;
    }
    const char* leopardToken = std::getenv("LEOPARD_TOKEN");
    if (leopardToken == NULL) {
        std::cout << "Leopard token does not exist in environment. export LEOPARD_TOKEN=\"TOKEN\"\n";
        environmentFail = true;
    }
    const char* elevenLabsToken = std::getenv("ELEVEN_LABS_TOKEN");
    if (elevenLabsToken == NULL) {
        std::cout << "Eleven labs token does not exist in environment. export ELEVEN_LABS_TOKEN=\"TOKEN\"\n";
        environmentFail = true;
    }
    if (environmentFail) return -1;

    ARGControl argParser;
    std::string characterPrompt;
    std::string speechID;
    argParser.addOption("-prompt", characterPrompt, "A character prompt (E.X. -> \"You are a helpful AI assistant.\")");
    argParser.addOption("-speech", speechID, "The Elevenlabs speech ID.");

    if (!argParser.parse(argc, argv)) {
        argParser.printHelp();
        return 1;
    }
    if (!characterPrompt.empty()) CHARACTER_PROMPT = characterPrompt.c_str();
    if (!speechID.empty()) ELEVEN_LABS_SPEECH_ID = speechID.c_str();

    // Kinda a large setup for settings but it's more neat this way...
    // However, only one object needed!
    ServiceInformation serviceSettings(openAIToken, leopardToken, elevenLabsToken, ELEVEN_LABS_SPEECH_ID);
    CharacterSettings characterSettings(CHARACTER_PROMPT, MIN_TIME_BETWEEN_RESPONSES_MILLISECONDS, BUFFER_SIZE_TRIGGER, MIN_TIMEOUT_RESPONSE, EMPTY_SPACE_TIMEOUT);
    AudioReceiverSettings audioReceiverSettings(MIN_BUFFER_SIZE, MAX_BUFFER_SIZE, TIMEOUT_MILLISECONDS);
    BotInformation settings(serviceSettings, characterSettings, audioReceiverSettings, CHECK_TIME_MILLISECONDS);

	CharacterBot bot(botToken, &settings);
	bot.run();

    std::cout << "End of main" << std::endl;
	return 0;
}