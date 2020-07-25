#include <iostream>
#include <memory>
#include <boost/asio.hpp>
#include <thread>
#include <vector>
#include "Client.hpp"
using namespace std;

int main() {
	std::shared_ptr<boost::asio::io_context> io { 
		std::make_shared<boost::asio::io_context>() 
	};
	
	Client client { io };
	client.Connect("127.0.0.1", 15001);
	vector<thread> ts;
	for(int i = 0; i < 2; i++) {
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

	while(true) {
		string str;
		getline(cin, str);
		client.Write(std::move(str));
	}

	for(auto& t: ts) {
		t.join();
	}
	return 0;
}
