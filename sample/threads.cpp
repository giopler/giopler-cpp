#include "giopler/giopler.hpp"
#include <cstdlib>
#include <thread>

// -----------------------------------------------------------------------------
void test(const int instance)
{
  giopler::dev::Function function(instance);
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  if (instance > 1) {
    test(instance-1);
  }
}

// -----------------------------------------------------------------------------
int main()
{
  test(1);
  std::thread t2 = std::thread(test,  1);
  std::thread t3 = std::thread(test,  3);
  std::thread t4 = std::thread(test, 10);
  t2.join();
  t3.join();
  t4.join();

  return EXIT_SUCCESS;
}
