#ifndef OPENMW_COMPONENTS_MYGUIPLATFORM_LOGLISTENER_H
#define OPENMW_COMPONENTS_MYGUIPLATFORM_LOGLISTENER_H

#include <string>
#include <boost/filesystem/fstream.hpp>

#include <MyGUI_ILogListener.h>
#include <MyGUI_LogSource.h>
#include <MyGUI_ConsoleLogListener.h>
#include <MyGUI_LevelLogFilter.h>

namespace osgMyGUI
{

    /// \brief  Custom MyGUI::ILogListener interface implementation
    /// being able to portably handle UTF-8 encoded path.
    /// \todo try patching MyGUI to make this easier
    class CustomLogListener : public MyGUI::ILogListener
    {
    public:
        CustomLogListener(const std::string &name)
            : mFileName(name)
        {}

        ~CustomLogListener() {}

        void open() override;
        void close() override;
        void flush() override;

        void log(const std::string& _section, MyGUI::LogLevel _level, const struct tm* _time, const std::string& _message, const char* _file, int _line) override;

        const std::string& getFileName() const { return mFileName; }

    private:
        boost::filesystem::ofstream mStream;
        std::string mFileName;
    };

    /// \brief Helper class holding data that required during
    /// MyGUI log creation
    class LogFacility
    {
        MyGUI::ConsoleLogListener  mConsole;
        CustomLogListener   mFile;
        MyGUI::LevelLogFilter      mFilter;
        MyGUI::LogSource           mSource;

    public:

        LogFacility(const std::string &output, bool console)
          : mFile(output)
        {
            mConsole.setEnabled(console);
            mFilter.setLoggingLevel(MyGUI::LogLevel::Info);

            mSource.addLogListener(&mFile);
            mSource.addLogListener(&mConsole);
            mSource.setLogFilter(&mFilter);

            mSource.open();
        }

        MyGUI::LogSource *getSource() { return &mSource; }
    };

}

#endif
