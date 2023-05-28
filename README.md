# Giopler

This is the C++20 client for the
[Giopler system (https://www.giopler.com/)](https://www.giopler.com/). Giopler is a profiling, debugging, and tracing system for C and C++. Please visit the website for much more information.

## Features

* Integrated profiling, debugging, and tracing
* C and C++ support
* Can be completely turned off to have near zero runtime overhead
* performance monitoring counters (PMC) values are exact (not sampling)

## Build Modes

| Name                     | Abbrev | Description                          |
|:-------------------------|:-------|:-------------------------------------|
| GIOPLER_BUILD_MODE_OFF   | Off    | turn off with near zero runtime cost |
| GIOPLER_BUILD_MODE_DEV   | Dev    | development and debugging            |
| GIOPLER_BUILD_MODE_TEST  | Test   | run unit tests                       |
| GIOPLER_BUILD_MODE_BENCH | Bench  | run benchmarking tests               |
| GIOPLER_BUILD_MODE_PROF  | Prof   | improve performance                  |
| GIOPLER_BUILD_MODE_QA    | Qa     | used by quality assurance team       |
| GIOPLER_BUILD_MODE_PROD  | Prod   | production deployments               |
