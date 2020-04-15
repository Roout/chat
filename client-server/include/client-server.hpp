#pragma once
#include <memory>
#include <mutex>
#include <vector>
#include <iostream>

namespace helper
{
	// Helper functions
	template<typename ...Args>
	void Write(std::mutex& m, Args&&... args) {
		(std::cerr<<...<<std::forward<Args>(args)) << std::endl;
	}
}

namespace wnet 
{



}
