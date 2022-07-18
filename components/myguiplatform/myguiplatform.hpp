#ifndef OPENMW_COMPONENTS_MYGUIPLATFORM_MYGUIPLATFORM_H
#define OPENMW_COMPONENTS_MYGUIPLATFORM_MYGUIPLATFORM_H

#include <string>
#include <memory>

#include <components/vfs/manager.hpp>

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
    class ImageManager;
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
        Platform(osgViewer::Viewer* viewer, osg::Group* guiRoot, Resource::ImageManager* imageManager, const VFS::Manager* vfs, float uiScalingFactor);

        ~Platform();

        void initialise(const std::string& resourcePath, const std::string& _logName = "MyGUI.log");

        void shutdown();

        RenderManager* getRenderManagerPtr();

        DataManager* getDataManagerPtr();

    private:
        std::unique_ptr<LogFacility> mLogFacility;
        std::unique_ptr<MyGUI::LogManager> mLogManager;
        std::unique_ptr<DataManager> mDataManager;
        std::unique_ptr<RenderManager> mRenderManager;
    };

}

#endif
