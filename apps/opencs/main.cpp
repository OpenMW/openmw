#include "editor.hpp"

#include <exception>
#include <string>

#include <QApplication>
#include <QIcon>
#include <QSurfaceFormat>

#include <osg/DisplaySettings>

#include <components/debug/debugging.hpp>
#include <components/debug/debuglog.hpp>
#include <components/platform/platform.hpp>

#include "model/doc/messages.hpp"
#include "model/world/universalid.hpp"

#ifdef Q_OS_MAC
#include <QDir>
#endif

Q_DECLARE_METATYPE(std::string)

class QEvent;
class QObject;

class Application : public QApplication
{
private:
    bool notify(QObject* receiver, QEvent* event) override
    {
        try
        {
            return QApplication::notify(receiver, event);
        }
        catch (const std::exception& exception)
        {
            Log(Debug::Error) << "An exception has been caught: " << exception.what();
        }

        return false;
    }

public:
    Application(int& argc, char* argv[])
        : QApplication(argc, argv)
    {
    }
};

void setQSurfaceFormat()
{
    osg::DisplaySettings* ds = osg::DisplaySettings::instance().get();
    QSurfaceFormat format = QSurfaceFormat::defaultFormat();
    format.setVersion(2, 1);
    format.setRenderableType(QSurfaceFormat::OpenGL);
    format.setDepthBufferSize(24);
    format.setSamples(ds->getMultiSamples());
    format.setStencilBufferSize(ds->getMinimumNumStencilBits());
    format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    QSurfaceFormat::setDefaultFormat(format);
}

int runApplication(int argc, char* argv[])
{
    Platform::init();

#ifdef Q_OS_MAC
    setenv("OSG_GL_TEXTURE_STORAGE", "OFF", 0);
#endif

    Q_INIT_RESOURCE(resources);

    qRegisterMetaType<std::string>("std::string");
    qRegisterMetaType<CSMWorld::UniversalId>("CSMWorld::UniversalId");
    qRegisterMetaType<CSMDoc::Message>("CSMDoc::Message");

    setQSurfaceFormat();
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);

    Application application(argc, argv);

    application.setWindowIcon(QIcon(":./openmw-cs.png"));

    CS::Editor editor(argc, argv);
#ifdef __linux__
    setlocale(LC_NUMERIC, "C");
#endif

    if (!editor.makeIPCServer())
    {
        editor.connectToIPCServer();
        return 0;
    }

    return editor.run();
}

int main(int argc, char* argv[])
{
    return wrapApplication(&runApplication, argc, argv, "OpenMW-CS");
}
