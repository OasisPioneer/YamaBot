#include <iostream>

#include "MessageProcessingCenter.HPP"

int main(int argc, char *argv[])
{
	const std::string BotToken = "";

	MessageProcessingCenter Bot;
	Bot.Run(BotToken);

	return 0;
}
