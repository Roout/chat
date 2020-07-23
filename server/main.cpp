#include <iostream>
#include <memory>
#include <boost/asio.hpp>
#include <thread>
#include <vector>
#include "server.hpp"
using namespace std;

int main() {
	std::shared_ptr<boost::asio::io_context> io { 
		std::make_shared<boost::asio::io_context>() 
	};

	Server server { io, 15001 };
	server.Start();
	vector<thread> ts;
	for(int i = 0; i < 2; i++) 
		ts.emplace_back([io]() {
				io->run();
		});
	io->run();
	
	return 0;
}