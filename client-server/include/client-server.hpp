#pragma once
#include <memory>
#include <mutex>
#include <vector>
#include <iostream>

namespace helper
{
	// Helper functions
	template<typename Head>
	void Write(std::mutex& m, Head&& context) {
		std::lock_guard<mutex> guard(m);
		std::cerr << std::forward<Head>(context) << " ";
	}
	template<typename Head, typename ...Tail>
	void Write(std::mutex& m, Head&& context, Tail&&... args) {
		(std::cerr<<...<<std::forward<Args>(args)) << std::endl;
	}
}

namespace wnet 
{



}
