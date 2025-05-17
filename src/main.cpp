#include <string>
#include <iostream>
#include <cstdlib>
#include "characterBot.h"
#include "ServiceHandler.h"

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

	CharacterBot bot(botToken, openAIToken, leopardToken, elevenLabsToken);
	bot.run();
	return 0;
}