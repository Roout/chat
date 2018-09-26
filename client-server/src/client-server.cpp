#include "client-server.h"
#include <thread>

using namespace std;

wnet::Client::Client() :
	m_io(make_shared<io_context>()),
	m_strand(make_shared<io_context::strand>(*m_io)),
	m_socket(*m_io)
{

}

void wnet::Client::Run()
{
	//@while(true) loop is for opportunity to handle exeption
	//and call again @io_context::run()
	while (true) {
		try {
			boost::system::error_code ec;
			m_io->run(ec);

			if (ec)
				wnet::Write(m_mutex, "Error code: ", ec, "\n");

			break; // to quite if work is done or @ec is set
		}
		catch (exception& e) {
			wnet::Write(m_mutex, "Caught exception:", e.what(), "\n");
			// if needed
			// throw; 
		}
	}
	

}

void wnet::Client::Test()
{
	vector<thread> threads;
	for (int i = 0; i < 2; i++)
		threads.emplace_back(bind(&Client::Run, this));
	
	try {
		tcp::resolver resolver(*m_io);
		tcp::resolver::query q("www.packtpub.com", "80");
		auto it = resolver.resolve(q);
		tcp::endpoint endpoint = *it;

		wnet::Write(m_mutex, "Connecting to: ", endpoint, "\n");

		m_socket.connect(endpoint);
		wnet::Write(m_mutex, "Connected\n");
	}
	catch (exception&ex) {
		wnet::Write(m_mutex, "Caught exception:", ex.what(), "\n");
		// if needed
		// throw; 
	}

	cin.get();
	boost::system::error_code ec;
	m_socket.shutdown(tcp::socket::shutdown_both, ec);
	m_socket.close(ec);

	m_io->stop();
	
	for (auto&t : threads) t.join();

}
