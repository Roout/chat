// client-server.cpp : Defines the entry point for the application.
//

#include "client-server.h"
#include <boost/lexical_cast.hpp>
using namespace std;

int main()
{
	cout << "Hello CMake." << endl;
	cin.get();

	cout << boost::lexical_cast<int>("1233");
	return 0;
}
