#ifndef DEBUG_DEBUGGING_H
#define DEBUG_DEBUGGING_H

#include <boost/filesystem/fstream.hpp>
#include <boost/iostreams/stream.hpp>

#include <components/files/configurationmanager.hpp>

#include <SDL_messagebox.h>

#include "debuglog.hpp"

#if defined _WIN32 && defined _DEBUG
#   undef WIN32_LEAN_AND_MEAN
#   define WIN32_LEAN_AND_MEAN
#   include <windows.h>
#endif

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

    class DebugOutputBase : public boost::iostreams::sink
    {
    public:
        DebugOutputBase()
        {
            if (CurrentDebugLevel == NoLevel)
                fillCurrentDebugLevel();
        }

        virtual std::streamsize write(const char *str, std::streamsize size);

        virtual ~DebugOutputBase() = default;

    protected:
        static Level getLevelMarker(const char *str);

        static void fillCurrentDebugLevel();

        virtual std::streamsize writeImpl(const char *str, std::streamsize size, Level debugLevel)
        {
            return size;
        }
    };

#ifdef _WIN32
    bool attachParentConsole();
#endif

#if defined _WIN32 && defined _DEBUG
    class DebugOutput : public DebugOutputBase
    {
    public:
        std::streamsize writeImpl(const char *str, std::streamsize size, Level debugLevel)
        {
            // Make a copy for null termination
            std::string tmp (str, static_cast<unsigned int>(size));
            // Write string to Visual Studio Debug output
            OutputDebugString (tmp.c_str ());
            return size;
        }

        virtual ~DebugOutput() {}
    };
#else

    class Tee : public DebugOutputBase
    {
    public:
        Tee(std::ostream &stream, std::ostream &stream2)
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

        virtual std::streamsize writeImpl(const char *str, std::streamsize size, Level debugLevel)
        {
            out.write (str, size);
            out.flush();

            if(mUseColor)
            {
                out2 << "\033[0;" << mColors[debugLevel] << "m";
                out2.write (str, size);
                out2 << "\033[0;" << Reset << "m";
            }
            else
            {
                out2.write(str, size);
            }
            out2.flush();

            return size;
        }

        virtual ~Tee() {}

    private:

        static bool useColoredOutput()
        {
    // Note: cmd.exe in Win10 should support ANSI colors, but in its own way.
#if defined(_WIN32)
            return 0;
#else
            char *term = getenv("TERM");
            bool useColor = term && !getenv("NO_COLOR") && isatty(fileno(stderr));

            return useColor;
#endif
        }

        std::ostream &out;
        std::ostream &out2;
        bool mUseColor;

        std::map<Level, int> mColors;
    };
#endif
}

int wrapApplication(int (*innerApplication)(int argc, char *argv[]), int argc, char *argv[], const std::string& appName);

#endif
