#include "debuglog.hpp"

namespace Debug
{
    Level CurrentDebugLevel = Level::NoLevel;
}

std::mutex Log::sLock;
