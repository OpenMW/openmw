#ifndef MISC_DEBUGGING_H
#define MISC_DEBUGGING_H

#include <boost/iostreams/stream.hpp>

#include <SDL_messagebox.h>

namespace Misc
{
#if defined(_WIN32) && defined(_DEBUG)

    class DebugOutput : public boost::iostreams::sink
    {
    public:
        std::streamsize write(const char *str, std::streamsize size)
        {
            // Make a copy for null termination
            std::string tmp (str, static_cast<unsigned int>(size));
            // Write string to Visual Studio Debug output
            OutputDebugString (tmp.c_str ());
            return size;
        }
    };
#else
    class Tee : public boost::iostreams::sink
    {
    public:
        Tee(std::ostream &stream, std::ostream &stream2)
            : out(stream), out2(stream2)
        {
        }

        std::streamsize write(const char *str, std::streamsize size)
        {
            out.write (str, size);
            out.flush();
            out2.write (str, size);
            out2.flush();
            return size;
        }

    private:
        std::ostream &out;
        std::ostream &out2;
    };
#endif
}

int wrapApplication(int (*innerApplication)(int argc, char *argv[]), int argc, char *argv[], const std::string& logName)
{
    // Some objects used to redirect cout and cerr
    // Scope must be here, so this still works inside the catch block for logging exceptions
    std::streambuf* cout_rdbuf = std::cout.rdbuf ();
    std::streambuf* cerr_rdbuf = std::cerr.rdbuf ();

#if !(defined(_WIN32) && defined(_DEBUG))
    boost::iostreams::stream_buffer<Misc::Tee> coutsb;
    boost::iostreams::stream_buffer<Misc::Tee> cerrsb;
#endif

    std::ostream oldcout(cout_rdbuf);
    std::ostream oldcerr(cerr_rdbuf);

    boost::filesystem::ofstream logfile;

    int ret = 0;
    try
    {
        Files::ConfigurationManager cfgMgr;
#if defined(_WIN32) && defined(_DEBUG)
        // Redirect cout and cerr to VS debug output when running in debug mode
        boost::iostreams::stream_buffer<Misc::DebugOutput> sb;
        sb.open(Misc::DebugOutput());
        std::cout.rdbuf (&sb);
        std::cerr.rdbuf (&sb);
#else
        // Redirect cout and cerr to the log file
        boost::filesystem::ofstream logfile;
        logfile.open (boost::filesystem::path(cfgMgr.getLogPath() / logName));

        std::ostream oldcout(cout_rdbuf);
        std::ostream oldcerr(cerr_rdbuf);

        coutsb.open (Misc::Tee(logfile, oldcout));
        cerrsb.open (Misc::Tee(logfile, oldcerr));

        std::cout.rdbuf (&coutsb);
        std::cerr.rdbuf (&cerrsb);
#endif
        ret = innerApplication(argc, argv);
    }
    catch (std::exception& e)
    {
#if (defined(__APPLE__) || defined(__linux) || defined(__unix) || defined(__posix))
        if (!isatty(fileno(stdin)))
#endif
            SDL_ShowSimpleMessageBox(0, "OpenMW: Fatal error", e.what(), NULL);

        std::cerr << "\nERROR: " << e.what() << std::endl;

        ret = 1;
    }

    // Restore cout and cerr
    std::cout.rdbuf(cout_rdbuf);
    std::cerr.rdbuf(cerr_rdbuf);

    return ret;
}

#endif
