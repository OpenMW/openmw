#include "debugging.hpp"

#include <chrono>
#include <deque>
#include <fstream>
#include <iostream>
#include <memory>

#ifdef _MSC_VER
// TODO: why is this necessary? this has /external:I
#pragma warning(push)
#pragma warning(disable : 4702)
#endif
#include <boost/iostreams/stream.hpp>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <components/crashcatcher/crashcatcher.hpp>
#include <components/files/conversion.hpp>
#include <components/misc/strings/conversion.hpp>
#include <components/misc/strings/lower.hpp>

#ifdef _WIN32
#include <components/crashcatcher/windows_crashcatcher.hpp>
#include <components/files/conversion.hpp>
#include <components/misc/windows.hpp>

#include <Knownfolders.h>

#pragma push_macro("FAR")
#pragma push_macro("NEAR")
#undef FAR
#define FAR
#undef NEAR
#define NEAR
#include <Shlobj.h>
#pragma pop_macro("NEAR")
#pragma pop_macro("FAR")

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

        // Note: Do not spend three days reinvestigating this PowerShell bug thinking its our bug.
        // https://gitlab.com/OpenMW/openmw/-/merge_requests/408#note_447467393
        // The handles look valid, but GetFinalPathNameByHandleA can't tell what files they go to and writing to them
        // doesn't work.

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
                std::cin.clear();
            }
            if (!outRedirected)
            {
                _wfreopen(L"CON", L"w", stdout);
                freopen("CON", "w", stdout);
                std::cout.clear();
            }
            if (!errRedirected)
            {
                _wfreopen(L"CON", L"w", stderr);
                freopen("CON", "w", stderr);
                std::cerr.clear();
            }

            return true;
        }

        return false;
    }
#endif

    static LogListener logListener;
    void setLogListener(LogListener listener)
    {
        logListener = std::move(listener);
    }

    namespace
    {
        class DebugOutputBase : public boost::iostreams::sink
        {
        public:
            virtual std::streamsize write(const char* str, std::streamsize size)
            {
                if (size <= 0)
                    return size;
                std::string_view msg{ str, static_cast<size_t>(size) };

                // Skip debug level marker
                Level level = All;
                if (Log::sWriteLevel)
                {
                    level = getLevelMarker(msg[0]);
                    msg = msg.substr(1);
                }

                char prefix[32];
                std::size_t prefixSize;
                {
                    prefix[0] = '[';
                    const auto now = std::chrono::system_clock::now();
                    const auto time = std::chrono::system_clock::to_time_t(now);
                    tm time_info{};
#ifdef _WIN32
                    (void)localtime_s(&time_info, &time);
#else
                    (void)localtime_r(&time, &time_info);
#endif
                    prefixSize = std::strftime(prefix + 1, sizeof(prefix) - 1, "%T", &time_info) + 1;
                    char levelLetter = " EWIVD*"[int(level)];
                    const auto ms
                        = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
                    prefixSize += snprintf(prefix + prefixSize, sizeof(prefix) - prefixSize, ".%03u %c] ",
                        static_cast<unsigned>(ms % 1000), levelLetter);
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
                        logListener(
                            level, std::string_view(prefix, prefixSize), std::string_view(msg.data(), lineSize));
                    msg = msg.substr(lineSize);
                }

                return size;
            }

            virtual ~DebugOutputBase() = default;

        protected:
            static Level getLevelMarker(char marker)
            {
                if (0 <= marker && static_cast<unsigned>(marker) < static_cast<unsigned>(All))
                    return static_cast<Level>(marker);
                return All;
            }

            virtual std::streamsize writeImpl(const char* str, std::streamsize size, Level debugLevel) { return size; }
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

        struct Record
        {
            std::string mValue;
            Level mLevel;
        };

        std::deque<Record> globalBuffer;

        Color getColor(Level level)
        {
            switch (level)
            {
                case Error:
                    return Red;
                case Warning:
                    return Yellow;
                case Info:
                    return Reset;
                case Verbose:
                    return DarkGray;
                case Debug:
                    return DarkGray;
                case All:
                    return Reset;
            }
            return Reset;
        }

        bool useColoredOutput()
        {
#if defined(_WIN32)
            if (std::getenv("NO_COLOR") != nullptr)
                return false;

            DWORD mode;
            if (GetConsoleMode(GetStdHandle(STD_ERROR_HANDLE), &mode) && mode & ENABLE_VIRTUAL_TERMINAL_PROCESSING)
                return true;

            // some console emulators may not use the Win32 API, so try the Unixy approach
            return std::getenv("TERM") != nullptr && GetFileType(GetStdHandle(STD_ERROR_HANDLE)) == FILE_TYPE_CHAR;
#else
            return std::getenv("TERM") != nullptr && std::getenv("NO_COLOR") == nullptr && isatty(fileno(stderr));
#endif
        }

        class Identity
        {
        public:
            explicit Identity(std::ostream& stream)
                : mStream(stream)
            {
            }

            void write(const char* str, std::streamsize size, Level /*level*/)
            {
                mStream.write(str, size);
                mStream.flush();
            }

        private:
            std::ostream& mStream;
        };

        class Coloured
        {
        public:
            explicit Coloured(std::ostream& stream)
                : mStream(stream)
                // TODO: check which stream is stderr?
                , mUseColor(useColoredOutput())
            {
            }

            void write(const char* str, std::streamsize size, Level level)
            {
                if (mUseColor)
                    mStream << "\033[0;" << getColor(level) << 'm';
                mStream.write(str, size);
                if (mUseColor)
                    mStream << "\033[0;" << Reset << 'm';
                mStream.flush();
            }

        private:
            std::ostream& mStream;
            bool mUseColor;
        };

        class Buffer
        {
        public:
            explicit Buffer(std::size_t capacity, std::deque<Record>& buffer)
                : mCapacity(capacity)
                , mBuffer(buffer)
            {
            }

            void write(const char* str, std::streamsize size, Level debugLevel)
            {
                while (mBuffer.size() >= mCapacity)
                    mBuffer.pop_front();
                mBuffer.push_back(Record{ std::string(str, size), debugLevel });
            }

        private:
            std::size_t mCapacity;
            std::deque<Record>& mBuffer;
        };

        template <class First, class Second>
        class Tee : public DebugOutputBase
        {
        public:
            explicit Tee(First first, Second second)
                : mFirst(first)
                , mSecond(second)
            {
            }

            std::streamsize writeImpl(const char* str, std::streamsize size, Level debugLevel) override
            {
                mFirst.write(str, size, debugLevel);
                mSecond.write(str, size, debugLevel);
                return size;
            }

        private:
            First mFirst;
            Second mSecond;
        };
#endif

        Level toLevel(std::string_view value)
        {
            if (value == "ERROR")
                return Error;
            if (value == "WARNING")
                return Warning;
            if (value == "INFO")
                return Info;
            if (value == "VERBOSE")
                return Verbose;
            if (value == "DEBUG")
                return Debug;

            return Verbose;
        }

        static std::unique_ptr<std::ostream> rawStdout = nullptr;
        static std::unique_ptr<std::ostream> rawStderr = nullptr;
        static std::unique_ptr<std::mutex> rawStderrMutex = nullptr;
        static std::ofstream logfile;

#if defined(_WIN32) && defined(_DEBUG)
        static boost::iostreams::stream_buffer<DebugOutput> sb;
#else
        static boost::iostreams::stream_buffer<Tee<Identity, Coloured>> standardOut;
        static boost::iostreams::stream_buffer<Tee<Identity, Coloured>> standardErr;
        static boost::iostreams::stream_buffer<Tee<Buffer, Coloured>> bufferedOut;
        static boost::iostreams::stream_buffer<Tee<Buffer, Coloured>> bufferedErr;
#endif
    }

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

    Level getDebugLevel()
    {
        if (const char* env = getenv("OPENMW_DEBUG_LEVEL"))
            return toLevel(env);

        return Verbose;
    }

    Level getRecastMaxLogLevel()
    {
        if (const char* env = getenv("OPENMW_RECAST_MAX_LOG_LEVEL"))
            return toLevel(env);

        return Error;
    }

    void setupLogging(const std::filesystem::path& logDir, std::string_view appName)
    {
        Log::sMinDebugLevel = getDebugLevel();
        Log::sWriteLevel = true;

#if !(defined(_WIN32) && defined(_DEBUG))
        const std::string logName = Misc::StringUtils::lowerCase(appName) + ".log";
        logfile.open(logDir / logName, std::ios::out);

        Identity log(logfile);

        for (const Record& v : globalBuffer)
            log.write(v.mValue.data(), v.mValue.size(), v.mLevel);

        globalBuffer.clear();

        standardOut.open(Tee(log, Coloured(*rawStdout)));
        standardErr.open(Tee(log, Coloured(*rawStderr)));

        std::cout.rdbuf(&standardOut);
        std::cerr.rdbuf(&standardErr);
#endif

#ifdef _WIN32
        if (Crash::CrashCatcher::instance())
        {
            Crash::CrashCatcher::instance()->updateDumpPath(logDir);
        }
#endif
    }

    int wrapApplication(
        int (*innerApplication)(int argc, char* argv[]), int argc, char* argv[], std::string_view appName)
    {
#if defined _WIN32
        (void)attachParentConsole();
#endif
        rawStdout = std::make_unique<std::ostream>(std::cout.rdbuf());
        rawStderr = std::make_unique<std::ostream>(std::cerr.rdbuf());
        rawStderrMutex = std::make_unique<std::mutex>();

#if defined(_WIN32) && defined(_DEBUG)
        // Redirect cout and cerr to VS debug output when running in debug mode
        sb.open(DebugOutput());
        std::cout.rdbuf(&sb);
        std::cerr.rdbuf(&sb);
#else
        constexpr std::size_t bufferCapacity = 1024;

        bufferedOut.open(Tee(Buffer(bufferCapacity, globalBuffer), Coloured(*rawStdout)));
        bufferedErr.open(Tee(Buffer(bufferCapacity, globalBuffer), Coloured(*rawStderr)));

        std::cout.rdbuf(&bufferedOut);
        std::cerr.rdbuf(&bufferedErr);
#endif

        int ret = 0;
        try
        {
            if (const auto env = std::getenv("OPENMW_DISABLE_CRASH_CATCHER");
                env == nullptr || Misc::StringUtils::toNumeric<int>(env, 0) == 0)
            {
#if defined(_WIN32)
                const std::string crashDumpName = Misc::StringUtils::lowerCase(appName) + "-crash.dmp";
                const std::string freezeDumpName = Misc::StringUtils::lowerCase(appName) + "-freeze.dmp";
                std::filesystem::path dumpDirectory = std::filesystem::temp_directory_path();
                PWSTR userProfile = nullptr;
                if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Profile, 0, nullptr, &userProfile)))
                {
                    dumpDirectory = userProfile;
                }
                CoTaskMemFree(userProfile);
                Crash::CrashCatcher crashy(argc, argv, dumpDirectory, crashDumpName, freezeDumpName);
#else
                const std::string crashLogName = Misc::StringUtils::lowerCase(appName) + "-crash.log";
                // install the crash handler as soon as possible.
                crashCatcherInstall(argc, argv, std::filesystem::temp_directory_path() / crashLogName);
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
                SDL_ShowSimpleMessageBox(0, (std::string(appName) + ": Fatal error").c_str(), e.what(), nullptr);

            Log(Debug::Error) << "Fatal error: " << e.what();

            ret = 1;
        }

        // Restore cout and cerr
        std::cout.rdbuf(rawStdout->rdbuf());
        std::cerr.rdbuf(rawStderr->rdbuf());

        Log::sMinDebugLevel = All;
        Log::sWriteLevel = false;

        return ret;
    }
}
