#include "giopler/giopler.hpp"
#include <iostream>
#include <cstdlib>

// -----------------------------------------------------------------------------
void there(int theres)
{
  giopler::dev::Function function;
  std::cout << "Hello, there!" << std::endl;
  if (--theres)   there(theres);
}

// -----------------------------------------------------------------------------
void world(int theres)
{
  giopler::dev::Function function;
  std::cout << "Hello, world!" << std::endl;
  there(2);
}

// -----------------------------------------------------------------------------
int main()
{
  giopler::dev::Function function;

  world(1);
  there(2);

  return EXIT_SUCCESS;
}
