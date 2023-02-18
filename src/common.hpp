#pragma once

/* NOTE: This gets included into every file */

#if defined(_WIN32)
#    define TARGET_WINDOWS
#    define _CRT_SECURE_NO_WARNINGS
#elif defined(__linux__)
#    define TARGET_LINUX
#endif

#include "common/config.hpp"
#include "common/macros.hpp"
#include "common/types.hpp"
