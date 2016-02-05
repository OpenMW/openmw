#include "myguiplatform.hpp"

#include "myguirendermanager.hpp"
#include "myguidatamanager.hpp"
#include "myguiloglistener.hpp"

namespace osgMyGUI
{

Platform::Platform(osgViewer::Viewer *viewer, osg::Group *guiRoot, Resource::ImageManager *imageManager, float uiScalingFactor)
    : mRenderManager(nullptr)
    , mDataManager(nullptr)
    , mLogManager(nullptr)
    , mLogFacility(nullptr)
{
    mLogManager = new MyGUI::LogManager();
    mRenderManager = new RenderManager(viewer, guiRoot, imageManager, uiScalingFactor);
    mDataManager = new DataManager();
}

Platform::~Platform()
{
    delete mRenderManager;
    mRenderManager = nullptr;
    delete mDataManager;
    mDataManager = nullptr;
    delete mLogManager;
    mLogManager = nullptr;
    delete mLogFacility;
    mLogFacility = nullptr;
}

void Platform::initialise(const std::string &resourcePath, const std::string &_logName)
{
    if (!_logName.empty() && !mLogFacility)
    {
        mLogFacility = new LogFacility(_logName, false);
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
    return mRenderManager;
}

DataManager *Platform::getDataManagerPtr()
{
    return mDataManager;
}


}
