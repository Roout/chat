// client-server.cpp : Defines the entry point for the application.
//
#include "client-server.h"
#include <boost/asio.hpp>
#include <boost/chrono.hpp>
#include <functional>
#include <thread>
using namespace std;

void TimerHandler(boost::asio::deadline_timer& timer, int time) {
	if (!time)
		cout << "Finish!\n";
	else {
		cout << ".." << time << endl;
		timer.expires_from_now(boost::posix_time::seconds(1));
		timer.async_wait(bind(&TimerHandler,ref(timer),--time));
	}
}

int main()
{
	boost::asio::io_context ioc;
	boost::asio::deadline_timer timer(ioc);
	boost::asio::post(ioc, bind(&TimerHandler, ref(timer), 3));
	thread t([&]() {
		ioc.run();
	});
	t.join();

	return 0;
}
