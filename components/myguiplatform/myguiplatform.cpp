#include "myguiplatform.hpp"

#include "myguidatamanager.hpp"
#include "myguiloglistener.hpp"
#include "myguirendermanager.hpp"

#include "components/files/conversion.hpp"

namespace MyGUIPlatform
{

    Platform::Platform(osgViewer::Viewer* viewer, osg::Group* guiRoot, Resource::ImageManager* imageManager,
        const VFS::Manager* vfs, float uiScalingFactor, const std::filesystem::path& resourcePath,
        const std::filesystem::path& logName)
        : mLogFacility(logName.empty() ? nullptr : std::make_unique<LogFacility>(logName, false))
        , mLogManager(std::make_unique<MyGUI::LogManager>())
        , mDataManager(std::make_unique<DataManager>(Files::pathToUnicodeString(resourcePath), vfs))
        , mRenderManager(std::make_unique<RenderManager>(viewer, guiRoot, imageManager, uiScalingFactor))
    {
        if (mLogFacility != nullptr)
            mLogManager->addLogSource(mLogFacility->getSource());

        mRenderManager->initialise();
    }

    Platform::~Platform() = default;

    void Platform::shutdown()
    {
        mRenderManager->shutdown();
    }

    RenderManager* Platform::getRenderManagerPtr()
    {
        return mRenderManager.get();
    }

    DataManager* Platform::getDataManagerPtr()
    {
        return mDataManager.get();
    }

}
