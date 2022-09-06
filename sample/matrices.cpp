/*
 *   Copyright 2022 Carlos Reyes
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */

// #include "brainyguy/bglogger.h"

// -------------------------------------------------------------------
// g++  -Ofast -march=native
//      -Wa,-adhln=matrices.s -g -fverbose-asm -masm=intel
//      -o matrices matrices.cpp
//      && sync && ./matrices

// cpupower frequency-info
// sudo cpupower frequency-set -u 3GHz

// -------------------------------------------------------------------

#include <emmintrin.h>   // intrinsics
#include <malloc.h>      // memalign()
#include <math.h>
#include <sched.h>       // sched_setaffinity()
#include <stdlib.h>      // exit()
#include <unistd.h>      // getpid()

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <ostream>
#include <random>
#include <string>

// -------------------------------------------------------------------
constexpr int MATRIX_DIM = 1024 * 8;   // can do up to 8
constexpr int CACHE_LINE_SIZE = 64;
constexpr int CACHE_LINE_DOUBLES = CACHE_LINE_SIZE / sizeof( double );
constexpr int MATRIX_ELEMENTS = MATRIX_DIM * MATRIX_DIM;
constexpr int MATRIX_SIZE = MATRIX_ELEMENTS * sizeof(double);

constexpr double CPU_FREQ = 3.0 * 1000000000.0;   // 3GHz
constexpr double DOUBLE_RANGE = 1000000.0;
constexpr double MIN_TIME_FUNC = 1.0;   // one second

// -------------------------------------------------------------------
void pinThread()
{
  cpu_set_t set;

  CPU_ZERO(&set);
  CPU_SET(0, &set);
  if (sched_setaffinity(getpid(), sizeof(set), &set) == -1)
    exit(EXIT_FAILURE);
}

// -------------------------------------------------------------------
template<typename F, typename... Args>
double funcTime(F func, Args&&... args)
{
  const std::chrono::high_resolution_clock::time_point startTime =
    std::chrono::high_resolution_clock::now();
  std::chrono::high_resolution_clock::time_point endTime = startTime;

  int times = 0;
  while (std::chrono::duration<double>(endTime - startTime).count() < MIN_TIME_FUNC) {
    ++times;
    func(std::forward<Args>(args)...);
    endTime = std::chrono::high_resolution_clock::now();
  }

  return std::chrono::duration<double>(endTime - startTime).count() / times;
}

// -------------------------------------------------------------------
// STL binary predicate
// Are two doubles are approximately equal?
// Knuth, Art of Computer Prog II, 1969, section 4.2.2 pages 217-218
bool approxEqual(const double x, const double y)
{
  return std::abs(x - y) <= 1e-5 * std::abs(x);
}

// -------------------------------------------------------------------
std::string humanNumber(int size)
{
  static const char *types[] = {"", "KB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB"};
  std::ostringstream oss;

  for (int i = 0, div = 1024; i < 9; ++i, size /= div) {
    if (size < div) {
      oss << size << types[i] << std::flush;
      return oss.str();
    }
  }

  return "";
}

// -------------------------------------------------------------------
// convert time in seconds into CPU cycles (3.9GHz)
double cycles(const double dim, const double secs)
{
    const double cycles = secs * CPU_FREQ;
    const double mults  = dim * dim * dim;
    return cycles / mults;
}

// -------------------------------------------------------------------
// -------------------------------------------------------------------
// bad order
__attribute__ ((noinline))
void
alg01(double *A,
      double *B,
      double *C,
      const int y1,
      const int x1y2,
      const int x2)
{
  std::fill(C, &C[y1*x2], 0.0);

  for (int i = 0; i < y1; ++i) {
    for (int j = 0; j < x2; ++j) {
      for (int k = 0; k < x1y2; ++k) {
        C[i*x1y2+j] += A[i*x1y2+k] * B[k*x1y2+j];
      }
    }
  }
}

// -------------------------------------------------------------------
// plain
__attribute__ ((noinline))
void
alg02(double *A,
      double *B,
      double *C,
      const int y1,
      const int x1y2,
      const int x2)
{
  std::fill(C, &C[y1*x2], 0.0);

  for (int i = 0; i < y1; ++i) {
    for (int k = 0; k < x1y2; ++k) {
      for (int j = 0; j < x2; ++j) {
    C[i*x1y2+j] += A[i*x1y2+k] * B[k*x1y2+j];
      }
    }
  }
}

// -------------------------------------------------------------------
// restrict pointer arguments
// http://stackoverflow.com/questions/1965487/does-the-restrict-keyword-provide-significant-benefits-in-gcc-g
// concludes a 9% speed advantage with restrict in gcc (2009)
__attribute__ ((noinline))
void
alg03(double *__restrict__ A,
      double *__restrict__ B,
      double *__restrict__ C,
      const int y1,
      const int x1y2,
      const int x2)
{
  std::fill(C, &C[y1*x2], 0.0);

  for (int i = 0; i < y1; ++i) {
    for (int k = 0; k < x1y2; ++k) {
      for (int j = 0; j < x2; ++j) {
        C[i*x1y2+j] += A[i*x1y2+k] * B[k*x1y2+j];
      }
    }
  }
}

// -------------------------------------------------------------------
// not blocked, work array (extra memory)
// http://www.netlib.org/utk/people/JackDongarra/CCDSC-2016/slides/talk34-walker.pdf
__attribute__ ((noinline))
void
alg04(double *__restrict__ A,
      double *__restrict__ B,
      double *__restrict__ C,
      const int y1,
      const int x1y2,
      const int x2)
{
  double*__restrict__ work =
    static_cast<double*>(__builtin_assume_aligned(memalign(CACHE_LINE_SIZE, x1y2*sizeof(double)),
                               CACHE_LINE_SIZE));
  if (reinterpret_cast<intptr_t>(work) % CACHE_LINE_SIZE)  exit(EXIT_FAILURE);

  for (int i = 0; i < y1; ++i) {
    for (int j = 0; j < x2; ++j)   work[j] = 0.0;

    for (int k = 0; k < x1y2; ++k) {
      const double a = A[i * x1y2 + k];

      for (int j = 0; j < x2; ++j) {
        const double b = B[k * x2 + j];
        work[j] += a * b;
      }
    }

    for (int j = 0; j < x2; ++j)   C[i * y1 + j] = work[j];
  }

  free(work);
}

// -------------------------------------------------------------------
// blocked, zero fill
// http://stackoverflow.com/questions/16115770/block-matrix-multiplication
__attribute__ ((noinline))
void
alg05(double *__restrict__ A,
      double *__restrict__ B,
      double *__restrict__ C,
      const int y1,
      const int x1y2,
      const int x2)
{
  std::fill(C, &C[y1*x2], 0.0);

  for (int jj=0; jj<x2; jj += CACHE_LINE_DOUBLES)
    for (int kk=0; kk<x1y2; kk += CACHE_LINE_DOUBLES)
      for (int i=0; i<y1; i++)
    for(int j = jj; j < ((jj+CACHE_LINE_DOUBLES)>x2 ? x2 : (jj+CACHE_LINE_DOUBLES)); j++) {
      double temp = 0.0;

      for (int k = kk; k<((kk+CACHE_LINE_DOUBLES)>x1y2 ? x1y2 : (kk+CACHE_LINE_DOUBLES)); k++)
        temp += A[i*x1y2+k] * B[k*x1y2+j];

      C[i*x1y2+j] += temp;
    }
}

// -------------------------------------------------------------------
// blocked, incremental clear
// http://www.cs.rochester.edu/users/faculty/sandhya/csc252/lectures/lecture-memopt.pdf
__attribute__ ((noinline))
void
alg06(double *__restrict__ A,
      double *__restrict__ B,
      double *__restrict__ C,
      const int y1,
      const int x1y2,
      const int x2)
{
  for (int jj=0; jj<x2; jj+=CACHE_LINE_DOUBLES) {
    for (int i=0; i<y1; i++)
      for (int j=jj; j < std::min(jj+CACHE_LINE_DOUBLES,x2); j++)
    C[i*x1y2+j] = 0.0;

    for (int kk=0; kk<x1y2; kk+=CACHE_LINE_DOUBLES)
      for (int i=0; i<y1; i++)
    for (int j=jj; j < std::min(jj+CACHE_LINE_DOUBLES,x2); j++) {
      double sum = 0.0;

      for (int k=kk; k < std::min(kk+CACHE_LINE_DOUBLES,x1y2); k++)
        sum += A[i*x1y2+k] * B[k*x1y2+j];

      C[i*x1y2+j] += sum;
    }
  }
}

// -------------------------------------------------------------------
// Drepper, SIMD intrinsics, blocked, prefetch
__attribute__ ((noinline))
void
alg07(double *__restrict__ A,
      double *__restrict__ B,
      double *__restrict__ C,
      const int y1,
      const int x1y2,
      const int x2)
{
  int i, j, k, i2, j2, k2;
  double* rres;
  double* rmul1;
  double* rmul2;

  std::fill(C, &C[y1*x2], 0.0);

  for (i = 0; i < y1; i += CACHE_LINE_DOUBLES)
    for (j = 0; j < x2; j += CACHE_LINE_DOUBLES)
      for (k = 0; k < x1y2; k += CACHE_LINE_DOUBLES)
    for (i2 = 0, rres = &C[i*x1y2+j], rmul1 = &A[i*x1y2+k];
         i2 < CACHE_LINE_DOUBLES;
         ++i2, rres += y1, rmul1 += y1)
      {
        _mm_prefetch(&rmul1[8], _MM_HINT_NTA);

        for (k2 = 0, rmul2 = &B[k*x1y2+j]; k2 < CACHE_LINE_DOUBLES; ++k2, rmul2 += x1y2)
          {
        __m128d m1d = _mm_load_sd(&rmul1[k2]);
        m1d = _mm_unpacklo_pd(m1d, m1d);

        for (j2 = 0; j2 < CACHE_LINE_DOUBLES; j2 += 2)
          {
            __m128d m2 = _mm_load_pd(&rmul2[j2]);
            __m128d r2 = _mm_load_pd(&rres[j2]);
            _mm_store_pd(&rres[j2],
                 _mm_add_pd(_mm_mul_pd(m2, m1d), r2));
          }
          }
      }
}

// -------------------------------------------------------------------
// Carlos, not blocked
__attribute__ ((noinline))
void
alg08(double *__restrict__ A,
      double *__restrict__ B,
      double *__restrict__ C,
      const int y1,
      const int x1y2,
      const int x2)
{
  const double *__restrict__ AA = static_cast<double*>(__builtin_assume_aligned(A, CACHE_LINE_SIZE));
  const double *__restrict__ BB = static_cast<double*>(__builtin_assume_aligned(B, CACHE_LINE_SIZE));
        double *__restrict__ CC = static_cast<double*>(__builtin_assume_aligned(C, CACHE_LINE_SIZE));

  for (int i = 0; i < y1; ++i)
    for (int k = 0; k < x1y2; ++k)
      for (int j = 0; j < x2; ++j)
        CC[i*x2+j] = (k ? CC[i*x2+j] : 0.0) + AA[i*x1y2+k] * BB[k*x2+j];
}

// -------------------------------------------------------------------
// Carlos, not blocked
__attribute__ ((noinline))
void
alg09(double *__restrict__ A,
      double *__restrict__ B,
      double *__restrict__ C,
      const int y1,
      const int x1y2,
      const int x2)
{
  const double *__restrict__ AA = static_cast<double*>(__builtin_assume_aligned(A, CACHE_LINE_SIZE));
  const double *__restrict__ BB = static_cast<double*>(__builtin_assume_aligned(B, CACHE_LINE_SIZE));
        double *__restrict__ CC = static_cast<double*>(__builtin_assume_aligned(C, CACHE_LINE_SIZE));

  double*__restrict__ W =
    static_cast<double*>(__builtin_assume_aligned(memalign(CACHE_LINE_SIZE, x1y2*sizeof(double)),
                               CACHE_LINE_SIZE));
  if (reinterpret_cast<intptr_t>(W) % CACHE_LINE_SIZE)  exit(EXIT_FAILURE);

  for (int i = 0; i < y1; ++i) {
    for (int j = 0; j < x2; ++j)
      W[j] = AA[i*x1y2+0] * BB[0*x2+j];   // k == 0

    for (int k = 1; k < x1y2; ++k)
      for (int j = 0; j < x2; ++j)
        W[j] += AA[i*x1y2+k] * BB[k*x2+j];

    for (int j = 0; j < x2; ++j)   CC[i*x2+j] = W[j];
  }

  free(W);
}

// -------------------------------------------------------------------
// Carlos, not blocked
__attribute__ ((noinline))
void
alg10(double *__restrict__ A,
      double *__restrict__ B,
      double *__restrict__ C,
      const int y1,
      const int x1y2,
      const int x2)
{
  for (int i = 0; i < y1; ++i) {
    for (int j = 0; j < x2; ++j)
      C[i*x2+j] = A[i*x1y2+0] * B[0*x2+j];   // k == 0

    for (int k = 1; k < x1y2; ++k)
      for (int j = 0; j < x2; ++j)
    C[i*x2+j] += A[i*x1y2+k] * B[k*x2+j];
  }
}

// -------------------------------------------------------------------
// -------------------------------------------------------------------
int main() {
  pinThread();

  std::random_device rand_dev;
  std::seed_seq seed{rand_dev(), rand_dev(), rand_dev(), rand_dev(),
      rand_dev(), rand_dev(), rand_dev(), rand_dev()};
  std::mt19937 rng(seed);
  std::uniform_real_distribution<double> distr(-DOUBLE_RANGE, DOUBLE_RANGE);

  double* A = static_cast<double*>(memalign(CACHE_LINE_SIZE, MATRIX_SIZE));
  if (reinterpret_cast<intptr_t>(A) % CACHE_LINE_SIZE)  exit(EXIT_FAILURE);
  std::generate(A, &A[MATRIX_ELEMENTS], [&]{ return distr(rng); });

  double* B = static_cast<double*>(memalign(CACHE_LINE_SIZE, MATRIX_SIZE));
  if (reinterpret_cast<intptr_t>(B) % CACHE_LINE_SIZE)  exit(EXIT_FAILURE);
  std::generate(B, &B[MATRIX_ELEMENTS], [&]{ return distr(rng); });

  double* C = static_cast<double*>(memalign(CACHE_LINE_SIZE, MATRIX_SIZE));
  if (reinterpret_cast<intptr_t>(C) % CACHE_LINE_SIZE)  exit(EXIT_FAILURE);

  double* T = static_cast<double*>(memalign(CACHE_LINE_SIZE, MATRIX_SIZE));
  if (reinterpret_cast<intptr_t>(T) % CACHE_LINE_SIZE)  exit(EXIT_FAILURE);

  std::cout << std::endl << std::setw(5)  << "dim"
        << std::setw(8) << "size"
        << std::setw(8) << "1"
        << std::setw(8) << "2"
        << std::setw(8) << "3"
        << std::setw(8) << "4"
        << std::setw(8) << "5"
        << std::setw(8) << "6"
        << std::setw(8) << "7"
        << std::setw(8) << "8"
        << std::setw(8) << "9"
        << std::setw(8) << "10"
        << std::endl;

  for (int array_dim = CACHE_LINE_DOUBLES;
       array_dim <= MATRIX_DIM;
       array_dim *= 2)
    {
      std::cout << std::setw(5) << array_dim
        << std::setw(8) << humanNumber(3*array_dim*array_dim*sizeof(double))
        << std::flush;

      std::fill(C, &C[array_dim*array_dim], DOUBLE_RANGE);
      std::cout    << std::setw(8) << std::fixed << std::setprecision(2)
        << cycles(array_dim, funcTime(alg01, A, B, C, array_dim, array_dim, array_dim))
        << std::flush;
      std::copy(C, &C[array_dim*array_dim], T);

      std::fill(C, &C[array_dim*array_dim], DOUBLE_RANGE);
      std::cout    << std::setw(8) << std::fixed << std::setprecision(2)
        << cycles(array_dim, funcTime(alg02, A, B, C, array_dim, array_dim, array_dim))
        << std::flush;
      if (!std::equal(C, &C[array_dim*array_dim], T, &T[array_dim*array_dim], approxEqual))
        exit(EXIT_FAILURE);

      std::fill(C, &C[array_dim*array_dim], DOUBLE_RANGE);
      std::cout    << std::setw(8) << std::fixed << std::setprecision(2)
        << cycles(array_dim, funcTime(alg03, A, B, C, array_dim, array_dim, array_dim))
        << std::flush;
      if (!std::equal(C, &C[array_dim*array_dim], T, &T[array_dim*array_dim], approxEqual))
        exit(EXIT_FAILURE);

      std::fill(C, &C[array_dim*array_dim], DOUBLE_RANGE);
      std::cout    << std::setw(8) << std::fixed << std::setprecision(2)
        << cycles(array_dim, funcTime(alg04, A, B, C, array_dim, array_dim, array_dim))
        << std::flush;
      if (!std::equal(C, &C[array_dim*array_dim], T, &T[array_dim*array_dim], approxEqual))
        exit(EXIT_FAILURE);

      std::fill(C, &C[array_dim*array_dim], DOUBLE_RANGE);
      std::cout    << std::setw(8) << std::fixed << std::setprecision(2)
        << cycles(array_dim, funcTime(alg05, A, B, C, array_dim, array_dim, array_dim))
        << std::flush;
      if (!std::equal(C, &C[array_dim*array_dim], T, &T[array_dim*array_dim], approxEqual))
        exit(EXIT_FAILURE);

      std::fill(C, &C[array_dim*array_dim], DOUBLE_RANGE);
      std::cout    << std::setw(8) << std::fixed << std::setprecision(2)
        << cycles(array_dim, funcTime(alg06, A, B, C, array_dim, array_dim, array_dim))
        << std::flush;
      if (!std::equal(C, &C[array_dim*array_dim], T, &T[array_dim*array_dim], approxEqual))
        exit(EXIT_FAILURE);

      std::fill(C, &C[array_dim*array_dim], DOUBLE_RANGE);
      std::cout    << std::setw(8) << std::fixed << std::setprecision(2)
        << cycles(array_dim, funcTime(alg07, A, B, C, array_dim, array_dim, array_dim))
        << std::flush;
      if (!std::equal(C, &C[array_dim*array_dim], T, &T[array_dim*array_dim], approxEqual))
        exit(EXIT_FAILURE);

      std::fill(C, &C[array_dim*array_dim], DOUBLE_RANGE);
      std::cout    << std::setw(8) << std::fixed << std::setprecision(2)
        << cycles(array_dim, funcTime(alg08, A, B, C, array_dim, array_dim, array_dim))
        << std::flush;
      if (!std::equal(C, &C[array_dim*array_dim], T, &T[array_dim*array_dim], approxEqual))
        exit(EXIT_FAILURE);

      std::fill(C, &C[array_dim*array_dim], DOUBLE_RANGE);
      std::cout    << std::setw(8) << std::fixed << std::setprecision(2)
        << cycles(array_dim, funcTime(alg09, A, B, C, array_dim, array_dim, array_dim))
        << std::flush;
      if (!std::equal(C, &C[array_dim*array_dim], T, &T[array_dim*array_dim], approxEqual))
        exit(EXIT_FAILURE);

      std::fill(C, &C[array_dim*array_dim], DOUBLE_RANGE);
      std::cout    << std::setw(8) << std::fixed << std::setprecision(2)
        << cycles(array_dim, funcTime(alg10, A, B, C, array_dim, array_dim, array_dim))
        << std::flush;
      if (!std::equal(C, &C[array_dim*array_dim], T, &T[array_dim*array_dim], approxEqual))
        exit(EXIT_FAILURE);

      std::cout << std::endl;
    }

  free(C);
  free(B);
  free(A);
  return 0;
}
