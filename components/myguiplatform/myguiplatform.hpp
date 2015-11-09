#ifndef OPENMW_COMPONENTS_MYGUIPLATFORM_MYGUIPLATFORM_H
#define OPENMW_COMPONENTS_MYGUIPLATFORM_MYGUIPLATFORM_H

#include <string>

namespace osgViewer
{
    class Viewer;
}
namespace osg
{
    class Group;
}
namespace Resource
{
    class TextureManager;
}
namespace MyGUI
{
    class LogManager;
}

namespace osgMyGUI
{

    class RenderManager;
    class DataManager;
    class LogFacility;

    class Platform
    {
    public:
        Platform(osgViewer::Viewer* viewer, osg::Group* guiRoot, Resource::TextureManager* textureManager, float uiScalingFactor);

        ~Platform();

        void initialise(const std::string& resourcePath, const std::string& _logName = "MyGUI.log");

        void shutdown();

        RenderManager* getRenderManagerPtr();

        DataManager* getDataManagerPtr();

    private:
        RenderManager* mRenderManager;
        DataManager* mDataManager;
        MyGUI::LogManager* mLogManager;
        LogFacility* mLogFacility;
    };

}

#endif
