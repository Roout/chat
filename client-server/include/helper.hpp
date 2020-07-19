#pragma once
#include <mutex>
#include <iostream>

namespace helper
{
	// Helper functions
	template<typename ...Args>
	void Write(std::mutex& m, Args&&... args) {
		std::lock_guard<std::mutex> guard(m);
		(std::cerr<<...<<std::forward<Args>(args)) << '\n';
	}
}