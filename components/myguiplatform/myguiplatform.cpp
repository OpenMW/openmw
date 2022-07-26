#include "myguiplatform.hpp"

#include "myguirendermanager.hpp"
#include "myguidatamanager.hpp"
#include "myguiloglistener.hpp"

namespace osgMyGUI
{

Platform::Platform(osgViewer::Viewer *viewer, osg::Group* guiRoot, Resource::ImageManager* imageManager,
    const VFS::Manager* vfs, float uiScalingFactor, const std::string& resourcePath, const std::string& logName)
    : mLogFacility(logName.empty() ? nullptr : std::make_unique<LogFacility>(logName, false))
    , mLogManager(std::make_unique<MyGUI::LogManager>())
    , mDataManager(std::make_unique<DataManager>(resourcePath, vfs))
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

RenderManager *Platform::getRenderManagerPtr()
{
    return mRenderManager.get();
}

DataManager *Platform::getDataManagerPtr()
{
    return mDataManager.get();
}

}
