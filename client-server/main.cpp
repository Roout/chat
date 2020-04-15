#include <boost/lexical_cast.hpp>
#include <iostream>
using namespace std;

int main() {
  cout << boost::lexical_cast<int>("123") << endl;
  return 0;
}
