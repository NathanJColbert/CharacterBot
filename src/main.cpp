#include <string>
#include <iostream>
#include <cstdlib>
#include "characterBot.h"
#include "ServiceHandler.h"
#include "ARGControl.h"

// MEH!
const char* CHARACTER_PROMPT = "Your name is Palamedes. You are a great and powerful fantasy knight. Respond as if you are an individual in a group. Do not speak in the third person. Keep your response short";
const char* ELEVEN_LABS_SPEECH_ID = "N2lVS1w4EtoT3dr4eOWO";
std::vector<std::string> KEY_WORDS = { "knight", "palamedes" };

//const char* CHARACTER_PROMPT = "You are a helpful programmer. You are there to guide your friends on their programming homework. Respond as if you are an individual in a group. Do not speak in the third person. Keep your response short.";
//const char* ELEVEN_LABS_SPEECH_ID = "JBFqnCBsd6RMkjVDRZzb";
//std::vector<const char*> KEY_WORDS = { "program", "help", "variable" };

//const char* CHARACTER_PROMPT = "Your name is Potion seller. You are a grummpy old man who refuses to sell potions to anyone. This is because you believe your potions are too strong for anyone. Respond as if you are an individual in a group. Do not speak in the third person. Keep your response short";
//const char* ELEVEN_LABS_SPEECH_ID = "zQtrcVyDViAdtX0qcTTG";
//std::vector<const char*> KEY_WORDS = { "potion", "buy", "purchase" };

//const char* CHARACTER_PROMPT = "Your name is Jarvis. You are a brilliant AI. Respond as if you are an individual in a group. Do not speak in the third person. Keep your response short";
//const char* ELEVEN_LABS_SPEECH_ID = "wDsJlOXPqcvIUKdLXjDs";
//std::vector<const char*> KEY_WORDS = { "jarvis", "help", "why" };

const long long CHECK_TIME_MILLISECONDS = 200;

const long long MIN_TIME_BETWEEN_RESPONSES_MILLISECONDS = 10000;
const size_t BUFFER_SIZE_TRIGGER = 100;
const long long MIN_TIMEOUT_RESPONSE = 900;
const long long EMPTY_SPACE_TIMEOUT = 10000;

const size_t MIN_BUFFER_SIZE = 8000;
const size_t MAX_BUFFER_SIZE = 3200000;
const long long TIMEOUT_MILLISECONDS = 700;

float RESPONSE_CHANCE = 10;

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
    float chance = -1;
    std::string keys;
    argParser.addOption("-prompt", characterPrompt, "A character prompt (E.X. -> \"You are a helpful AI assistant.\")");
    argParser.addOption("-speech", speechID, "The Elevenlabs speech ID.");
    argParser.addOption("-response_chance", chance, "The chance that the bot will respond a text channel.");
    argParser.addOption("-keywords", keys, "Keywords that the bot will 100% respond to in the a text channel. Written as: \"keyword1|keyword2|keywordn\"");

    if (!argParser.parse(argc, argv)) {
        argParser.printHelp();
        return 1;
    }
    if (!characterPrompt.empty()) CHARACTER_PROMPT = characterPrompt.c_str();
    if (!speechID.empty()) ELEVEN_LABS_SPEECH_ID = speechID.c_str();
    if (chance != -1) RESPONSE_CHANCE = chance;
    if (!keys.empty()) {
        KEY_WORDS.clear();
        std::string current;
        for (int i = 0; i < keys[i]; i++) {
            if (keys[i] == '|') {
                KEY_WORDS.push_back(current);
                current.clear();
                continue;
            }
            current += keys[i];
        }
        if (!current.empty())
            KEY_WORDS.push_back(current);
    }

    //---------------------------------------------------
    // You know you love this! <333333
    //---------------------------------------------------
    ServiceInformation serviceSettings(openAIToken, leopardToken, elevenLabsToken, ELEVEN_LABS_SPEECH_ID);
    CharacterSettings characterSettings(CHARACTER_PROMPT, MIN_TIME_BETWEEN_RESPONSES_MILLISECONDS, BUFFER_SIZE_TRIGGER, MIN_TIMEOUT_RESPONSE, EMPTY_SPACE_TIMEOUT);
    AudioReceiverSettings audioReceiverSettings(MIN_BUFFER_SIZE, MAX_BUFFER_SIZE, TIMEOUT_MILLISECONDS);
    BotInformation settings(serviceSettings, characterSettings, audioReceiverSettings, CHECK_TIME_MILLISECONDS);
    MessageContextSettings messageSettings(openAIToken, CHARACTER_PROMPT, KEY_WORDS, RESPONSE_CHANCE);

	CharacterBot bot(botToken, &settings, &messageSettings);
	bot.run();

    std::cout << "End of main" << std::endl;
	return 0;
}