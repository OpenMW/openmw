#include "myguiloglistener.hpp"

#include <iomanip>

#include <components/debug/debuglog.hpp>

namespace MyGUIPlatform
{
    void CustomLogListener::open()
    {
        mStream.open(mFileName, std::ios_base::out);
        if (!mStream.is_open())
            Log(Debug::Error) << "Unable to create MyGUI log with path " << mFileName;
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

    void CustomLogListener::log(std::string_view _section, MyGUI::LogLevel _level, const struct tm* _time,
        std::string_view _message, std::string_view _file, int _line)
    {
        if (mStream.is_open())
        {
            std::string_view separator = "  |  ";
            mStream << std::setw(2) << std::setfill('0') << _time->tm_hour << ":" << std::setw(2) << std::setfill('0')
                    << _time->tm_min << ":" << std::setw(2) << std::setfill('0') << _time->tm_sec << separator
                    << _section << separator << _level.print() << separator << _message << separator << _file
                    << separator << _line << std::endl;
        }
    }

    MyGUI::LogLevel LogFacility::getCurrentLogLevel() const
    {
        switch (Log::sMinDebugLevel)
        {
            case Debug::Error:
                return MyGUI::LogLevel::Error;
            case Debug::Warning:
                return MyGUI::LogLevel::Warning;
            default:
                return MyGUI::LogLevel::Info;
        }
    }
}
