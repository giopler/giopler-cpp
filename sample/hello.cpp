#include "giopler/giopler.hpp"
#include <iostream>
#include <cstdlib>

// -----------------------------------------------------------------------------
void hello(int hellos)
{
  giopler::dev::Function function(hellos);
  giopler::dev::confirm(hellos);
  std::cout << "Hello, world!" << std::endl;
  giopler::dev::line("printed hello");
  if (--hellos)   hello(hellos);
}

// -----------------------------------------------------------------------------
int main()
{
  giopler::dev::Function function;
  hello(2);
  giopler::prod::branch("done with hellos");

  return EXIT_SUCCESS;
}
