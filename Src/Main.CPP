#include <iostream>

#include "MessageProcessingCenter.HPP"

int main(int argc, char *argv[])
{
	const std::string BotToken = "7491395783:AAHDyV70HZlDNKK5IDFfK0irgQWQLBIhOHs";

	MessageProcessingCenter Bot;
	Bot.Run(BotToken);

	return 0;
}
