#include "debugging.hpp"

#include <components/crashcatcher/crashcatcher.hpp>

#ifdef _WIN32
#   undef WIN32_LEAN_AND_MEAN
#   define WIN32_LEAN_AND_MEAN
#   include <windows.h>
#endif

namespace Debug
{
#ifdef _WIN32
    bool attachParentConsole()
    {
        if (GetConsoleWindow() != nullptr)
            return true;

        if (AttachConsole(ATTACH_PARENT_PROCESS))
        {
            fflush(stdout);
            fflush(stderr);
            std::cout.flush();
            std::cerr.flush();

            // this looks dubious but is really the right way
            _wfreopen(L"CON", L"w", stdout);
            _wfreopen(L"CON", L"w", stderr);
            _wfreopen(L"CON", L"r", stdin);
            freopen("CON", "w", stdout);
            freopen("CON", "w", stderr);
            freopen("CON", "r", stdin);

            return true;
        }

        return false;
    }
#endif

    std::streamsize DebugOutputBase::write(const char *str, std::streamsize size)
    {
        // Skip debug level marker
        Level level = getLevelMarker(str);
        if (level != NoLevel)
        {
            writeImpl(str+1, size-1, level);
            return size;
        }

        writeImpl(str, size, NoLevel);
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

int wrapApplication(int (*innerApplication)(int argc, char *argv[]), int argc, char *argv[], const std::string& appName)
{
#if defined _WIN32
    (void)Debug::attachParentConsole();
#endif

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
    const std::string crashLogName = Misc::StringUtils::lowerCase(appName) + "-crash.log";
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
        logfile.open (boost::filesystem::path(cfgMgr.getLogPath() / logName));

        coutsb.open (Debug::Tee(logfile, oldcout));
        cerrsb.open (Debug::Tee(logfile, oldcerr));

        std::cout.rdbuf (&coutsb);
        std::cerr.rdbuf (&cerrsb);
#endif

        // install the crash handler as soon as possible. note that the log path
        // does not depend on config being read.
        crashCatcherInstall(argc, argv, (cfgMgr.getLogPath() / crashLogName).string());

        ret = innerApplication(argc, argv);
    }
    catch (std::exception& e)
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

    return ret;
}
