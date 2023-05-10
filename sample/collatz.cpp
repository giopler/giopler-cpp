#include "giopler/giopler.hpp"
#include <iostream>
#include <cstdlib>
#include <type_traits>

// -----------------------------------------------------------------------------
// is the provided integer an even value?
// https://en.cppreference.com/w/cpp/types/is_integral
template <class T>
constexpr bool is_even(const T i) {
  static_assert(std::is_integral<T>::value, "Integral required.");
  return !(i & 1);
}

// -----------------------------------------------------------------------------
// compute the Collatz conjecture:
//   if number is even, next term is n/2
//   Otherwise the next term is 3*n+1
// https://en.wikipedia.org/wiki/Collatz_conjecture
// returns the number of steps taken to reach one
int collatz(int number)
{
  giopler::dev::Function function;

  if (number == 1)
    return 0;         // done
  else if (is_even(number))
    return 1+collatz(number >> 1);
  else
    return 1+collatz(3*number + 1);
}

// -----------------------------------------------------------------------------
int main()
{
  giopler::dev::Function function;

  // number of steps to reach one from the index starting value
  const int steps[] = { 0,0, 1, 7, 2, 5, 8, 16, 3, 19,
                        6, 14, 9, 9, 17, 17, 4, 12, 20,
                        20, 7, 7, 15, 15, 10, 23, 10, 111,
                        18, 18, 18, 106, 5, 26, 13, 13, 21,
                        21, 21, 34, 8, 109, 8, 29, 16, 16,
                        16, 104, 11, 24, 24 };
  constexpr int total_steps = sizeof(steps) / sizeof(int);

  for (int n = 1; n < total_steps; ++n) {
    try {
      giopler::dev::confirm(collatz(n) == steps[n]);
    } catch (const giopler::contract_violation &error) {
      std::cerr << "collatz: index=" << n << " " << error.what() << std::endl;
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}