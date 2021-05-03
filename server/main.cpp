#include <iostream>
#include <memory>
#include <thread>
#include <exception>
#include <vector>

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

#include "Server.hpp"

int main() {
	std::shared_ptr<boost::asio::io_context> io { 
		std::make_shared<boost::asio::io_context>() 
	};

	Server server { io, 15001 };
	server.Start();
	std::vector<std::thread> ts;
	for (int i = 0; i < 4; i++) {
		ts.emplace_back([io]() {
			for (;;) {
				try {
					io->run();
					break; // run() exited normally
				}
				catch (const std::exception& e) {
					std::cerr << "ERROR: " << e.what() << '\n';
				}
			}
		});
	}
	
	for (auto& t: ts) {
		t.join();
	}
	
	return EXIT_SUCCESS;
}
