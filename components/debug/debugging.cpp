#include "debugging.hpp"

#include <chrono>
#include <memory>
#include <functional>
#include <fstream>
#include <filesystem>

#include <components/crashcatcher/crashcatcher.hpp>
#include <components/files/configurationmanager.hpp>
#ifdef _WIN32
#include <components/crashcatcher/windows_crashcatcher.hpp>
#include <components/windows.hpp>
#endif

#include <SDL_messagebox.h>

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

    static LogListener logListener;
    void setLogListener(LogListener listener) { logListener = std::move(listener); }

    class DebugOutputBase : public boost::iostreams::sink
    {
    public:
        DebugOutputBase()
        {
            if (CurrentDebugLevel == NoLevel)
                fillCurrentDebugLevel();
        }

        virtual std::streamsize write(const char* str, std::streamsize size)
        {
            if (size <= 0)
                return size;
            std::string_view msg{ str, size_t(size) };

            // Skip debug level marker
            Level level = getLevelMarker(str);
            if (level != NoLevel)
                msg = msg.substr(1);

            char prefix[32];
            int prefixSize;
            {
                prefix[0] = '[';
                const auto now = std::chrono::system_clock::now();
                const auto time = std::chrono::system_clock::to_time_t(now);
                prefixSize = std::strftime(prefix + 1, sizeof(prefix) - 1, "%T", std::localtime(&time)) + 1;
                char levelLetter = " EWIVD*"[int(level)];
                const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
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
                if (logListener)
                    logListener(level, std::string_view(prefix, prefixSize), std::string_view(msg.data(), lineSize));
                msg = msg.substr(lineSize);
            }

            return size;
        }

        virtual ~DebugOutputBase() = default;

    protected:
        static Level getLevelMarker(const char* str)
        {
            if (unsigned(*str) <= unsigned(Marker))
            {
                return Level(*str);
            }

            return NoLevel;
        }

        static void fillCurrentDebugLevel()
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

        virtual std::streamsize writeImpl(const char* str, std::streamsize size, Level debugLevel)
        {
            return size;
        }
    };

#if defined _WIN32 && defined _DEBUG
    class DebugOutput : public DebugOutputBase
    {
    public:
        std::streamsize writeImpl(const char* str, std::streamsize size, Level debugLevel)
        {
            // Make a copy for null termination
            std::string tmp(str, static_cast<unsigned int>(size));
            // Write string to Visual Studio Debug output
            OutputDebugString(tmp.c_str());
            return size;
        }

        virtual ~DebugOutput() = default;
    };
#else

    class Tee : public DebugOutputBase
    {
    public:
        Tee(std::ostream& stream, std::ostream& stream2)
            : out(stream), out2(stream2)
        {
            // TODO: check which stream is stderr?
            mUseColor = useColoredOutput();

            mColors[Error] = Red;
            mColors[Warning] = Yellow;
            mColors[Info] = Reset;
            mColors[Verbose] = DarkGray;
            mColors[Debug] = DarkGray;
            mColors[NoLevel] = Reset;
        }

        std::streamsize writeImpl(const char* str, std::streamsize size, Level debugLevel) override
        {
            out.write(str, size);
            out.flush();

            if (mUseColor)
            {
                out2 << "\033[0;" << mColors[debugLevel] << "m";
                out2.write(str, size);
                out2 << "\033[0;" << Reset << "m";
            }
            else
            {
                out2.write(str, size);
            }
            out2.flush();

            return size;
        }

        virtual ~Tee() = default;

    private:

        static bool useColoredOutput()
        {
            // Note: cmd.exe in Win10 should support ANSI colors, but in its own way.
#if defined(_WIN32)
            return 0;
#else
            char* term = getenv("TERM");
            bool useColor = term && !getenv("NO_COLOR") && isatty(fileno(stderr));

            return useColor;
#endif
        }

        std::ostream& out;
        std::ostream& out2;
        bool mUseColor;

        std::map<Level, int> mColors;
    };
#endif

}

static std::unique_ptr<std::ostream> rawStdout = nullptr;
static std::unique_ptr<std::ostream> rawStderr = nullptr;
static std::unique_ptr<std::mutex> rawStderrMutex = nullptr;
static std::ofstream logfile;

#if defined(_WIN32) && defined(_DEBUG)
static boost::iostreams::stream_buffer<Debug::DebugOutput> sb;
#else
static boost::iostreams::stream_buffer<Debug::Tee> coutsb;
static boost::iostreams::stream_buffer<Debug::Tee> cerrsb;
#endif

std::ostream& getRawStdout()
{
    return rawStdout ? *rawStdout : std::cout;
}

std::ostream& getRawStderr()
{
    return rawStderr ? *rawStderr : std::cerr;
}

Misc::Locked<std::ostream&> getLockedRawStderr()
{
    return Misc::Locked<std::ostream&>(*rawStderrMutex, getRawStderr());
}

// Redirect cout and cerr to the log file
void setupLogging(const std::string& logDir, const std::string& appName, std::ios_base::openmode mode)
{
#if defined(_WIN32) && defined(_DEBUG)
    // Redirect cout and cerr to VS debug output when running in debug mode
    sb.open(Debug::DebugOutput());
    std::cout.rdbuf(&sb);
    std::cerr.rdbuf(&sb);
#else
    const std::string logName = Misc::StringUtils::lowerCase(appName) + ".log";
    logfile.open(std::filesystem::path(logDir) / logName, mode);

    coutsb.open(Debug::Tee(logfile, *rawStdout));
    cerrsb.open(Debug::Tee(logfile, *rawStderr));

    std::cout.rdbuf(&coutsb);
    std::cerr.rdbuf(&cerrsb);
#endif
}

int wrapApplication(int (*innerApplication)(int argc, char *argv[]), int argc, char *argv[],
                    const std::string& appName, bool autoSetupLogging)
{
#if defined _WIN32
    (void)Debug::attachParentConsole();
#endif
    rawStdout = std::make_unique<std::ostream>(std::cout.rdbuf());
    rawStderr = std::make_unique<std::ostream>(std::cerr.rdbuf());
    rawStderrMutex = std::make_unique<std::mutex>();

    int ret = 0;
    try
    {
        Files::ConfigurationManager cfgMgr;

        if (autoSetupLogging)
        {
            std::ios_base::openmode mode = std::ios::out;

            // If we are collecting a stack trace, append to existing log file
            if (argc == 2 && strcmp(argv[1], crash_switch) == 0)
                mode |= std::ios::app;

            setupLogging(cfgMgr.getLogPath().string(), appName, mode);
        }

        if (const auto env = std::getenv("OPENMW_DISABLE_CRASH_CATCHER"); env == nullptr || std::atol(env) == 0)
        {
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
        else
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
    std::cout.rdbuf(rawStdout->rdbuf());
    std::cerr.rdbuf(rawStderr->rdbuf());
    Debug::CurrentDebugLevel = Debug::NoLevel;

    return ret;
}
