#pragma once
#include <boost/asio.hpp>
#include <memory>
#include <mutex>
#include <vector>
#include <iostream>

namespace wnet 
{
	using std::shared_ptr;

	using boost::asio::io_context;
	using boost::asio::ip::tcp;


	template<typename Head>
	void Write(std::mutex& m, Head&& context) {
		std::lock_guard<mutex> guard(m);
		std::cerr << std::forward<Head>(context) << " ";
	}

	template<typename Head, typename ...Tail>
	void Write(std::mutex& m, Head&& context, Tail&&... args) {
		//c++11
		{// additional scope to avoid recursive lock
			std::lock_guard<mutex> guard(m);
			std::cerr << std::forward<Head>(context) << " ";
		} 
		Write(m, std::forward<Tail>(args)...);
		//c++17 style:
		//(std::cerr<<...<<std::forward<Args>(args))<< std::endl;
	}

	class Client {
	public:
		Client();

		void Run();

		void Test();

	private:
		shared_ptr<io_context>	m_io;
		shared_ptr<io_context::strand>	m_strand;
		
		tcp::socket	m_socket;
		
		std::mutex	m_mutex;
	};
}
