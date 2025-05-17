#include <string>
#include <iostream>
#include <cstdlib>
#include "characterBot.h"
#include "ServiceHandler.h"

const char* CHARACTER_PROMPT = "Your name is Palamedes. You are a great and powerful fantasy knight. You are in a discord call with your friends. Respond as if you are an individual in a group. Do not speak in the third person. Keep your response short";
const char* ELEVEN_LABS_SPEECH_ID = "N2lVS1w4EtoT3dr4eOWO";

const long long MIN_TIME_BETWEEN_RESPONSES_MILLISECONDS = 10000;
const size_t BUFFER_SIZE_TRIGGER = 70;

const size_t MIN_BUFFER_SIZE = 10000;
const size_t MAX_BUFFER_SIZE = 3200000;
const long long TIMEOUT_MILLISECONDS = 700;

int main() {
    // Protect your keys!!!
    const char* botToken = std::getenv("BOT_TOKEN");
    if (botToken == NULL) {
        std::cout << "Bot token does not exist in environment. export BOT_TOKEN=\"TOKEN\"\n";
        return 1;
    }
    const char* openAIToken = std::getenv("OPEN_AI_TOKEN");
    if (openAIToken == NULL) {
        std::cout << "Open AI token does not exist in environment. export OPEN_AI_TOKEN=\"TOKEN\"\n";
        return 1;
    }
    const char* leopardToken = std::getenv("LEOPARD_TOKEN");
    if (leopardToken == NULL) {
        std::cout << "Leopard token does not exist in environment. export LEOPARD_TOKEN=\"TOKEN\"\n";
        return 1;
    }
    const char* elevenLabsToken = std::getenv("ELEVEN_LABS_TOKEN");
    if (elevenLabsToken == NULL) {
        std::cout << "Eleven labs token does not exist in environment. export ELEVEN_LABS_TOKEN=\"TOKEN\"\n";
        return 1;
    }

    ServiceInformation serviceSettings(openAIToken, leopardToken, elevenLabsToken, ELEVEN_LABS_SPEECH_ID);
    CharacterSettings characterSettings(CHARACTER_PROMPT, MIN_TIME_BETWEEN_RESPONSES_MILLISECONDS, BUFFER_SIZE_TRIGGER);
    AudioReceiverSettings audioReceiverSettings(MIN_BUFFER_SIZE, MAX_BUFFER_SIZE, TIMEOUT_MILLISECONDS);
    BotInformation settings(serviceSettings, characterSettings, audioReceiverSettings);

	CharacterBot bot(botToken, &settings);
	bot.run();
	return 0;
}