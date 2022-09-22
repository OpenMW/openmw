#ifndef DEBUG_DEBUGGING_H
#define DEBUG_DEBUGGING_H

#include <filesystem>
#include <functional>

#include <components/misc/guarded.hpp>

#include "debuglog.hpp"

namespace Debug
{
    // ANSI colors for terminal
    enum Color
    {
        Reset = 0,
        DarkGray = 90,
        Red = 91,
        Yellow = 93
    };

#ifdef _WIN32
    bool attachParentConsole();
#endif

    using LogListener = std::function<void(Debug::Level, std::string_view prefix, std::string_view msg)>;
    void setLogListener(LogListener);
}

// Can be used to print messages without timestamps
std::ostream& getRawStdout();

std::ostream& getRawStderr();

Misc::Locked<std::ostream&> getLockedRawStderr();

void setupLogging(
    const std::filesystem::path& logDir, const std::string& appName, std::ios_base::openmode mode = std::ios::out);

int wrapApplication(int (*innerApplication)(int argc, char* argv[]), int argc, char* argv[], const std::string& appName,
    bool autoSetupLogging = true);

#endif
