#pragma once

#if defined(_WIN32)
#    define TARGET_WINDOWS
#elif defined(__linux__)
#    define TARGET_LINUX
#endif

/* clang-format off */

#include "common/config.hpp"
#include "common/macros.hpp"
#include "common/types.hpp"
#include "common/opengl.hpp"
