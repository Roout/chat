#include <iostream>
#include <memory>
#include <thread>
#include <vector>

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

#include "Client.hpp"

using namespace std;

int main() {
	std::shared_ptr<boost::asio::io_context> io { 
		std::make_shared<boost::asio::io_context>() 
	};
	std::shared_ptr<boost::asio::ssl::context> sslContext { 
		std::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::sslv23) 
	};

	boost::system::error_code error;
	sslContext->load_verify_file("ca.pem", error);
	if (error) {
		std::cerr << "Can't find ca.pem file.\n";
		exit(1);
	}
	
	auto client = std::make_shared<Client>(io, sslContext);
	client->Connect("127.0.0.1", "15001");
	vector<thread> ts;
	for (int i = 0; i < 2; i++) {
		ts.emplace_back([io]() {
			for (;;) {
				try {
					io->run();
					break; // run() exited normally
				}
				catch (...) {
					// Deal with exception as appropriate.
				}
			}
		});
	}
	
	cin.ignore();

	while (true) {
		string str;
		getline(cin, str);
		client->Write(std::move(str));
	}

	for (auto& t: ts) {
		t.join();
	}

	return EXIT_SUCCESS;
}
