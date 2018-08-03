#ifndef MISC_DEBUGGING_H
#define MISC_DEBUGGING_H

#include <boost/iostreams/stream.hpp>

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

#endif
