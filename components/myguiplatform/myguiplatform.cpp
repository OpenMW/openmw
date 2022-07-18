#include "myguiplatform.hpp"

#include "myguirendermanager.hpp"
#include "myguidatamanager.hpp"
#include "myguiloglistener.hpp"

namespace osgMyGUI
{

Platform::Platform(osgViewer::Viewer *viewer, osg::Group *guiRoot, Resource::ImageManager *imageManager,
    const VFS::Manager* vfs, float uiScalingFactor)
    : mLogFacility(nullptr)
    , mLogManager(std::make_unique<MyGUI::LogManager>())
    , mDataManager(std::make_unique<DataManager>(vfs))
    , mRenderManager(std::make_unique<RenderManager>(viewer, guiRoot, imageManager, uiScalingFactor))
{
}

Platform::~Platform() {}

void Platform::initialise(const std::string &resourcePath, const std::string &_logName)
{
    if (!_logName.empty() && !mLogFacility)
    {
        mLogFacility = std::make_unique<LogFacility>(_logName, false);
        mLogManager->addLogSource(mLogFacility->getSource());
    }

    mDataManager->setResourcePath(resourcePath);

    mRenderManager->initialise();
    mDataManager->initialise();
}

void Platform::shutdown()
{
    mRenderManager->shutdown();
    mDataManager->shutdown();
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
