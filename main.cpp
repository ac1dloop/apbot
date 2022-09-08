#include "bot.hpp"

int main(int argc, char** argv) {

	//std::cout.sync_with_stdio(false);

	if (argc < 3) {
		std::cout << "usage: " << argv[0] << " [api_id] [api_hash]" << std::endl;
		return 1;
	}

	APP::Bot bot{ std::atoi(argv[1]), argv[2] };

	bot.run();

	return 0;
}