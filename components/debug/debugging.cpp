#include "debugging.hpp"

#include <chrono>
#include <memory>
#include <functional>

#include <components/crashcatcher/crashcatcher.hpp>

#ifdef _WIN32
#   include <components/crashcatcher/windows_crashcatcher.hpp>
#   undef WIN32_LEAN_AND_MEAN
#   define WIN32_LEAN_AND_MEAN
#   include <windows.h>
#endif

namespace Debug
{
#ifdef _WIN32
    bool isRedirected(DWORD nStdHandle)
    {
        DWORD fileType = GetFileType(GetStdHandle(nStdHandle));

        return (fileType == FILE_TYPE_DISK) || (fileType == FILE_TYPE_PIPE);
    }

    bool attachParentConsole()
    {
        if (GetConsoleWindow() != nullptr)
            return true;

        bool inRedirected = isRedirected(STD_INPUT_HANDLE);
        bool outRedirected = isRedirected(STD_OUTPUT_HANDLE);
        bool errRedirected = isRedirected(STD_ERROR_HANDLE);

        if (AttachConsole(ATTACH_PARENT_PROCESS))
        {
            fflush(stdout);
            fflush(stderr);
            std::cout.flush();
            std::cerr.flush();

            // this looks dubious but is really the right way
            if (!inRedirected)
            {
                _wfreopen(L"CON", L"r", stdin);
                freopen("CON", "r", stdin);
            }
            if (!outRedirected)
            {
                _wfreopen(L"CON", L"w", stdout);
                freopen("CON", "w", stdout);
            }
            if (!errRedirected)
            {
                _wfreopen(L"CON", L"w", stderr);
                freopen("CON", "w", stderr);
            }

            return true;
        }

        return false;
    }
#endif

    std::streamsize DebugOutputBase::write(const char *str, std::streamsize size)
    {
        if (size <= 0)
            return size;
        std::string_view msg{str, size_t(size)};

        // Skip debug level marker
        Level level = getLevelMarker(str);
        if (level != NoLevel)
            msg = msg.substr(1);

        char prefix[32];
        int prefixSize;
        {
            prefix[0] = '[';
            uint64_t ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::high_resolution_clock::now().time_since_epoch()).count();
            std::time_t t = ms / 1000;
            prefixSize = std::strftime(prefix + 1, sizeof(prefix) - 1, "%T", std::localtime(&t)) + 1;
            char levelLetter = " EWIVD*"[int(level)];
            prefixSize += snprintf(prefix + prefixSize, sizeof(prefix) - prefixSize,
                                   ".%03u %c] ", static_cast<unsigned>(ms % 1000), levelLetter);
        }

        while (!msg.empty())
        {
            if (msg[0] == 0)
                break;
            size_t lineSize = 1;
            while (lineSize < msg.size() && msg[lineSize - 1] != '\n')
                lineSize++;
            writeImpl(prefix, prefixSize, level);
            writeImpl(msg.data(), lineSize, level);
            msg = msg.substr(lineSize);
        }

        return size;
    }

    Level DebugOutputBase::getLevelMarker(const char *str)
    {
        if (unsigned(*str) <= unsigned(Marker))
        {
            return Level(*str);
        }

        return NoLevel;
    }

    void DebugOutputBase::fillCurrentDebugLevel()
    {
        const char* env = getenv("OPENMW_DEBUG_LEVEL");
        if (env)
        {
            std::string value(env);
            if (value == "ERROR")
                CurrentDebugLevel = Error;
            else if (value == "WARNING")
                CurrentDebugLevel = Warning;
            else if (value == "INFO")
                CurrentDebugLevel = Info;
            else if (value == "VERBOSE")
                CurrentDebugLevel = Verbose;
            else if (value == "DEBUG")
                CurrentDebugLevel = Debug;

            return;
        }

        CurrentDebugLevel = Verbose;
    }
}

static std::unique_ptr<std::ostream> rawStdout = nullptr;

std::ostream& getRawStdout()
{
    return rawStdout ? *rawStdout : std::cout;
}

int wrapApplication(int (*innerApplication)(int argc, char *argv[]), int argc, char *argv[], const std::string& appName)
{
#if defined _WIN32
    (void)Debug::attachParentConsole();
#endif
    rawStdout = std::make_unique<std::ostream>(std::cout.rdbuf());

    // Some objects used to redirect cout and cerr
    // Scope must be here, so this still works inside the catch block for logging exceptions
    std::streambuf* cout_rdbuf = std::cout.rdbuf ();
    std::streambuf* cerr_rdbuf = std::cerr.rdbuf ();

#if defined(_WIN32) && defined(_DEBUG)
    boost::iostreams::stream_buffer<Debug::DebugOutput> sb;
#else
    boost::iostreams::stream_buffer<Debug::Tee> coutsb;
    boost::iostreams::stream_buffer<Debug::Tee> cerrsb;
    std::ostream oldcout(cout_rdbuf);
    std::ostream oldcerr(cerr_rdbuf);
#endif

    const std::string logName = Misc::StringUtils::lowerCase(appName) + ".log";
    boost::filesystem::ofstream logfile;

    int ret = 0;
    try
    {
        Files::ConfigurationManager cfgMgr;

#if defined(_WIN32) && defined(_DEBUG)
        // Redirect cout and cerr to VS debug output when running in debug mode
        sb.open(Debug::DebugOutput());
        std::cout.rdbuf (&sb);
        std::cerr.rdbuf (&sb);
#else
        // Redirect cout and cerr to the log file
        // If we are collecting a stack trace, append to existing log file
        std::ios_base::openmode mode = std::ios::out;
        if(argc == 2 && strcmp(argv[1], crash_switch) == 0)
            mode |= std::ios::app;

        logfile.open (boost::filesystem::path(cfgMgr.getLogPath() / logName), mode);

        coutsb.open (Debug::Tee(logfile, oldcout));
        cerrsb.open (Debug::Tee(logfile, oldcerr));

        std::cout.rdbuf (&coutsb);
        std::cerr.rdbuf (&cerrsb);
#endif

#if defined(_WIN32)
        const std::string crashLogName = Misc::StringUtils::lowerCase(appName) + "-crash.dmp";
        Crash::CrashCatcher crashy(argc, argv, (cfgMgr.getLogPath() / crashLogName).make_preferred().string());
#else
        const std::string crashLogName = Misc::StringUtils::lowerCase(appName) + "-crash.log";
        // install the crash handler as soon as possible. note that the log path
        // does not depend on config being read.
        crashCatcherInstall(argc, argv, (cfgMgr.getLogPath() / crashLogName).string());
#endif
        ret = innerApplication(argc, argv);
    }
    catch (const std::exception& e)
    {
#if (defined(__APPLE__) || defined(__linux) || defined(__unix) || defined(__posix))
        if (!isatty(fileno(stdin)))
#endif
            SDL_ShowSimpleMessageBox(0, (appName + ": Fatal error").c_str(), e.what(), nullptr);

        Log(Debug::Error) << "Error: " << e.what();

        ret = 1;
    }

    // Restore cout and cerr
    std::cout.rdbuf(cout_rdbuf);
    std::cerr.rdbuf(cerr_rdbuf);
    Debug::CurrentDebugLevel = Debug::NoLevel;

    return ret;
}
