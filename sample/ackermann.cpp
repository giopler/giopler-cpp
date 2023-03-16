

#include "giopler/giopler.hpp"
#include <cstdlib>

// -----------------------------------------------------------------------------
// https://en.wikipedia.org/wiki/Ackermann_function
// runs in about 9.5 seconds in debug mode for (4, 1)
uint64_t ackermann(const uint64_t m, const uint64_t n) {
  giopler::dev::Function function(m*n);
  if (m == 0)   return n+1;
  if (n == 0)   return ackermann(m-1, 1);
  return ackermann(m - 1, ackermann(m, n-1));
}

// -----------------------------------------------------------------------------
int main()
{
  giopler::dev::confirm(ackermann(0, 1) == 2);
  giopler::dev::confirm(ackermann(2, 0) == ackermann(1, 1));
  giopler::dev::confirm(ackermann(4, 0) == 13);
  giopler::dev::confirm(ackermann(3, 4) == 125);
  giopler::dev::confirm(ackermann(4, 1) == 65533);

  return EXIT_SUCCESS;
}
