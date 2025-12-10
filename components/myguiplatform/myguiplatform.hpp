#ifndef OPENMW_COMPONENTS_MYGUIPLATFORM_MYGUIPLATFORM_H
#define OPENMW_COMPONENTS_MYGUIPLATFORM_MYGUIPLATFORM_H

#include <filesystem>
#include <memory>
#include <string>

#include <components/vfs/pathutil.hpp>

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
namespace VFS
{
    class Manager;
}

namespace MyGUIPlatform
{

    class RenderManager;
    class DataManager;
    class LogFacility;

    class Platform
    {
    public:
        Platform(osgViewer::Viewer* viewer, osg::Group* guiRoot, Resource::ImageManager* imageManager,
            const VFS::Manager* vfs, float uiScalingFactor, VFS::Path::NormalizedView resourcePath,
            const std::filesystem::path& logName = "MyGUI.log");

        ~Platform();

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
