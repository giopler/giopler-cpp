#include <atomic>
#include <deque>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <source_location>
#include <sstream>
#include <stack>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>

#include <cstdlib>
#include <cstring>
#include <cerrno>


int test(const int instance)
{
  std::cerr << "inside test " << instance << std::endl;
  if (instance > 1) {
    test(instance-1);
  }
  return 0;
}

int
main() {
  //const int t1_result = test();
  std::thread t2 = std::thread(test, 1);
  std::thread t3 = std::thread(test, 2);
  std::thread t4 = std::thread(test, 3);
  t2.join();
  t3.join();
  t4.join();
}
