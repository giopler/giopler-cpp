# CMake find_package module for the Google Brotli compression utility
# https://github.com/google/brotli/

include(FindPackageHandleStandardArgs)

find_path(BROTLI_INCLUDE_DIR "brotli/encode.h")

find_library(BROTLICOMMON_LIBRARY NAMES brotlicommon)
find_library(BROTLIENC_LIBRARY    NAMES brotlienc)
find_library(BROTLIDEC_LIBRARY    NAMES brotlidec)

find_package_handle_standard_args(Brotli
    FOUND_VAR
      BROTLI_FOUND
    REQUIRED_VARS
      BROTLIENC_LIBRARY
      BROTLIDEC_LIBRARY
      BROTLICOMMON_LIBRARY
      BROTLI_INCLUDE_DIR
    FAIL_MESSAGE
      "Could NOT find BROTLI compression program and library"
)

set(BROTLI_INCLUDE_DIRS ${BROTLI_INCLUDE_DIR})
set(BROTLI_LIBRARIES ${BROTLICOMMON_LIBRARY} ${BROTLIENC_LIBRARY} ${BROTLIDEC_LIBRARY})
