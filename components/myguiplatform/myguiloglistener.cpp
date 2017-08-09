#include "myguiloglistener.hpp"

#include <iomanip>

namespace osgMyGUI
{
    void CustomLogListener::open()
    {
        mStream.open(boost::filesystem::path(mFileName), std::ios_base::out);
    }

    void CustomLogListener::close()
    {
        if (mStream.is_open())
            mStream.close();
    }

    void CustomLogListener::flush()
    {
        if (mStream.is_open())
            mStream.flush();
    }

    void CustomLogListener::log(const std::string& _section, MyGUI::LogLevel _level, const struct tm* _time, const std::string& _message, const char* _file, int _line)
    {
        if (mStream.is_open())
        {
            const char* separator = "  |  ";
            mStream << std::setw(2) << std::setfill('0') << _time->tm_hour << ":"
                << std::setw(2) << std::setfill('0') << _time->tm_min << ":"
                << std::setw(2) << std::setfill('0') << _time->tm_sec << separator
                << _section << separator << _level.print() << separator
                << _message << separator << _file << separator << _line << std::endl;
        }
    }
}
