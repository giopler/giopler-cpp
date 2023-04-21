#include "giopler/giopler.hpp"
#include <cstdlib>

// -----------------------------------------------------------------------------
// https://en.wikipedia.org/wiki/Fibonacci_sequence
int fibonacci(int n)
{
  giopler::dev::Function function(n);
  if (n <= 1)   return n;
  return fibonacci(n - 1) + fibonacci(n - 2);
}

// -----------------------------------------------------------------------------
int main()
{
  giopler::dev::confirm(fibonacci( 1) ==    1);
  giopler::dev::confirm(fibonacci(10) ==   55);
  giopler::dev::confirm(fibonacci(13) ==  233);
  giopler::dev::confirm(fibonacci(19) == 4181);

  return EXIT_SUCCESS;
}
