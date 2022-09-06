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

// https://en.wikipedia.org/wiki/Ackermann_function
// runs in about 9.5 seconds in debug mode for (4, 1)
uint64_t ackermann(const uint64_t m, const uint64_t n) {
    if (m == 0)   return n+1;
    if (n == 0)   return ackermann(m-1, 1);
    return ackermann(m - 1, ackermann(m, n-1));
}

// bg_test(ackermann, ackermann4_1, {
//     const uint64_t result = ackermann(4, 1);
//     return !(result == 65533);
// })

int main(int argc, const char** argv, const char** envp) {
    // bg_function("", "", 0, {
    //     bg_add_sink("stderr", "all", "json header comments", NULL, NULL);
    //     bg_add_sink("stderr", "all", "spaces header comments", NULL, NULL);
    // });

    return 0;
}
