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

#include "gioppler/gioppler.hpp"

// https://en.wikipedia.org/wiki/Matrix_multiplication

// L1d cache:                       192 KiB (6 instances)
// L2 cache:                        3 MiB (6 instances)
// L3 cache:                        32 MiB (1 instance)

// matrix dimension size for fitting into L1 cache - doubles
#define L1_DIM_DOUBLE     32

// matrix dimension size for fitting into L1 cache - floats
#define L1_DIM_FLOAT      64

// matrix dimension size for fitting into L2 cache - doubles
#define L2_DIM_DOUBLE     128

// matrix dimension size for fitting into L2 cache - floats
#define L2_DIM_FLOAT      256

// matrix dimension size for fitting into L3 cache - doubles
#define L3_DIM_DOUBLE     418

// matrix dimension size for fitting into L3 cache - floats
#define L3_DIM_FLOAT      836

// matrix dimension size for fitting into main memory - doubles
#define MAIN_DIM_DOUBLE     1024

// matrix dimension size for fitting into main memory - floats
#define MAIN_DIM_FLOAT      2048

// -----------------------------------------------------------------------------
enum {NUM_ITERATIONS = 100, CPU_CACHE_LINE_SIZE = 64};

// -----------------------------------------------------------------------------
void
matrix_set_double(double* matrix, const size_t rows, const size_t cols,
                  const size_t row, const size_t col, const double new_value) {
  ((void)rows);
  matrix[row*cols+col] = new_value;
}

// -----------------------------------------------------------------------------
double
matrix_get_double(double* matrix, const size_t rows, const size_t cols,
                  const size_t row, const size_t col) {
  ((void)rows);
  return matrix[row*cols+col];
}

// -----------------------------------------------------------------------------
// 0 <= values <= 1
void
matrix_rand_double(double* matrix, const size_t rows, const size_t cols) {
  for (size_t row = 0; row < rows; ++row) {
    for (size_t col = 0; col < cols; ++col) {
      matrix_set_double(matrix, rows, cols, row, col, (double)rand() / RAND_MAX);
    }
  }
}

// -----------------------------------------------------------------------------
void
matrix_zeros_double(double* matrix, const size_t rows, const size_t cols) {
  for (size_t row = 0; row < rows; ++row) {
    for (size_t col = 0; col < cols; ++col) {
      matrix_set_double(matrix, rows, cols, row, col, 0.0);
    }
  }
}

// -----------------------------------------------------------------------------
bool
equal_matrices_double(double* X, double* Y, const size_t elements) {
  for (size_t element = 0; element < elements; ++element) {
    if (!bg_approx_equal_double(*(X++), *(Y++))) {
      return false;
    }
  }
  return true;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// x_row, y_col, k; zeroing R one element at a time
void
alg01_double(double *X,
             double *Y,
             double *R,
             const size_t x_rows,
             const size_t x_cols,
             const size_t y_rows,
             const size_t y_cols) {
  bg_assert(x_cols == y_rows);
  const size_t x_cols_y_rows = x_cols;

  for (size_t x_row = 0; x_row < x_rows; ++x_row) {
    for (size_t y_col = 0; y_col < y_cols; ++y_col) {
      double sum = 0.0;
      for (size_t x_col_y_row = 0; x_col_y_row < x_cols_y_rows; ++x_col_y_row) {
        sum += X[x_row*x_rows+x_col_y_row] * Y[x_col_y_row*y_rows+y_col];
      }
      R[x_row*x_rows+y_col] = sum;
    }
  }
}

// -----------------------------------------------------------------------------
// x_row, k, y_col; zeroing R first
void
alg02_double(double *X,
             double *Y,
             double *R,
             const size_t x_rows,
             const size_t x_cols,
             const size_t y_rows,
             const size_t y_cols) {
  bg_assert(x_cols == y_rows);
  const size_t x_cols_y_rows = x_cols;
  matrix_zeros_double(R, x_rows, y_cols);

  for (size_t x_row = 0; x_row < x_rows; ++x_row) {
    for (size_t x_col_y_row = 0; x_col_y_row < x_cols_y_rows; ++x_col_y_row) {
      for (size_t y_col = 0; y_col < y_cols; ++y_col) {
        R[x_row*x_rows+y_col] +=
          X[x_row*x_rows+x_col_y_row] * Y[x_col_y_row*y_rows+y_col];
      }
    }
  }
}

// -----------------------------------------------------------------------------
void matrix_mult_l1_double() {
  bg_function("l1.double", "", L1_DIM_DOUBLE, {
    const size_t x_rows = L1_DIM_DOUBLE;
    const size_t x_cols = L1_DIM_DOUBLE;
    const size_t y_rows = L1_DIM_DOUBLE;
    const size_t y_cols = L1_DIM_DOUBLE;
    bg_assert(x_cols == y_rows);

    double* X = aligned_alloc(CPU_CACHE_LINE_SIZE, x_rows*x_cols*sizeof(double));
    bg_assert(X);
    double* Y = aligned_alloc(CPU_CACHE_LINE_SIZE, y_rows*y_cols*sizeof(double));
    bg_assert(Y);
    double* R1 = aligned_alloc(CPU_CACHE_LINE_SIZE, x_rows*y_cols*sizeof(double));
    bg_assert(R1);
    double* R2 = aligned_alloc(CPU_CACHE_LINE_SIZE, x_rows*y_cols*sizeof(double));
    bg_assert(R2);

    matrix_rand_double(X, x_rows, x_cols);
    matrix_rand_double(Y, y_rows, y_cols);

    for (int iterations = 0; iterations < NUM_ITERATIONS; ++iterations) {
      alg01_double(X, Y, R1, x_rows, x_cols, y_rows, y_cols);
      alg02_double(X, Y, R2, x_rows, x_cols, y_rows, y_cols);
      bg_assert(equal_matrices_double(R1, R2, x_rows*y_cols));
    }

    free(X);
    free(Y);
    free(R1);
    free(R2);
  });
}

// -----------------------------------------------------------------------------
int main(int argc, const char** argv, const char** envp) {
  bg_function( "", "", 0, {
    matrix_mult_l1_double();
  });
  return 0;
}
