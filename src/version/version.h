#pragma once

#include "version.hpp"


namespace WinuxCmd {
// Re-inline all symbols from version.hpp
using ::WinuxCmd::VERSION_MAJOR;
using ::WinuxCmd::VERSION_MINOR;
using ::WinuxCmd::VERSION_PATCH;
using ::WinuxCmd::VERSION_STRING;
using ::WinuxCmd::VERSION_NUMBER;
using ::WinuxCmd::BUILD_TYPE;
using ::WinuxCmd::BUILD_TIMESTAMP;
using ::WinuxCmd::COMPILER_ID;
using ::WinuxCmd::COMPILER_VERSION;
}  // namespace WinuxCmd
