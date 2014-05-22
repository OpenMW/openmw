#ifndef OPENENGINE_MYGUI_LOGLISTENER_H
#define OPENENGINE_MYGUI_LOGLISTENER_H

#include <string>
#include <boost/filesystem/fstream.hpp>

#include <MyGUI_ILogListener.h>

namespace MyGUI
{
    /// \brief  Custom MyGUI::ILogListener interface implementation
    /// being able to portably handle UTF-8 encoded path.
    class CustomLogListener : public ILogListener
    {
    public:
        CustomLogListener(const std::string &name)
            : mFileName(name)
        {}

        ~CustomLogListener() {}

        virtual void open();
        virtual void close();
        virtual void flush();

        virtual void log(const std::string& _section, LogLevel _level, const struct tm* _time, const std::string& _message, const char* _file, int _line);

        const std::string& getFileName() const { return mFileName; }

    private:
        boost::filesystem::ofstream mStream;
        std::string mFileName;
    };

}

#endif
