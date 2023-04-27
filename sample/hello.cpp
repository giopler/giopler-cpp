#include "giopler/giopler.hpp"
#include <iostream>
#include <cstdlib>

// -----------------------------------------------------------------------------
void there()
{
  giopler::dev::Function function;
  std::cout << "Hello, there!" << std::endl;
}

// -----------------------------------------------------------------------------
void hello()
{
  giopler::dev::Function function;
  std::cout << "Hello, world!" << std::endl;
  there();
}

// -----------------------------------------------------------------------------
int main()
{
  giopler::dev::Function function;

  hello();
  there();

  return EXIT_SUCCESS;
}
