#include "editor.hpp"

#include <exception>
#include <iostream>
#include <string>

#include <QApplication>
#include <QIcon>
#include <QMetaType>

#include <components/misc/debugging.hpp>

#include "model/doc/messages.hpp"
#include "model/world/universalid.hpp"

#include <SDL_messagebox.h>

#ifdef Q_OS_MAC
#include <QDir>
#endif

Q_DECLARE_METATYPE (std::string)

class Application : public QApplication
{
    private:

        bool notify (QObject *receiver, QEvent *event)
        {
            try
            {
                return QApplication::notify (receiver, event);
            }
            catch (const std::exception& exception)
            {
                std::cerr << "An exception has been caught: " << exception.what() << std::endl;
            }

            return false;
        }

    public:

        Application (int& argc, char *argv[]) : QApplication (argc, argv) {}
};

int main(int argc, char *argv[])
{
    #ifdef Q_OS_MAC
        setenv("OSG_GL_TEXTURE_STORAGE", "OFF", 0);
    #endif

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
        // Redirect cout and cerr to openmw.log
        logfile.open (boost::filesystem::path(cfgMgr.getLogPath() / "/openmw-cs.log"));

        coutsb.open (Misc::Tee(logfile, oldcout));
        cerrsb.open (Misc::Tee(logfile, oldcerr));

        std::cout.rdbuf (&coutsb);
        std::cerr.rdbuf (&cerrsb);
#endif

        // To allow background thread drawing in OSG
        QApplication::setAttribute(Qt::AA_X11InitThreads, true);

        Q_INIT_RESOURCE (resources);

        qRegisterMetaType<std::string> ("std::string");
        qRegisterMetaType<CSMWorld::UniversalId> ("CSMWorld::UniversalId");
        qRegisterMetaType<CSMDoc::Message> ("CSMDoc::Message");

        Application application (argc, argv);

    #ifdef Q_OS_MAC
        QDir dir(QCoreApplication::applicationDirPath());
        QDir::setCurrent(dir.absolutePath());
    #endif

        application.setWindowIcon (QIcon (":./openmw-cs.png"));

        CS::Editor editor(argc, argv);

        if(!editor.makeIPCServer())
        {
            editor.connectToIPCServer();
            return 0;
        }
        ret = editor.run();
    }
    catch (std::exception& e)
    {
#if (defined(__APPLE__) || defined(__linux) || defined(__unix) || defined(__posix))
        if (!isatty(fileno(stdin)))
#endif
            SDL_ShowSimpleMessageBox(0, "OpenMW-CS: Fatal error", e.what(), NULL);

        std::cerr << "\nERROR: " << e.what() << std::endl;

        ret = 1;
    }

    // Restore cout and cerr
    std::cout.rdbuf(cout_rdbuf);
    std::cerr.rdbuf(cerr_rdbuf);

    return ret;
}
