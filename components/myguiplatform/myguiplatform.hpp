#ifndef OPENMW_COMPONENTS_MYGUIPLATFORM_MYGUIPLATFORM_H
#define OPENMW_COMPONENTS_MYGUIPLATFORM_MYGUIPLATFORM_H

#include "MyGUI_Prerequest.h"
#include "MyGUI_LogManager.h"

#include "myguirendermanager.hpp"
#include "myguidatamanager.hpp"
#include "myguiloglistener.hpp"

namespace osgMyGUI
{

    class Platform
    {
    public:
        Platform(osgViewer::Viewer* viewer, osg::Group* guiRoot, Resource::TextureManager* textureManager, float uiScalingFactor)
            : mRenderManager(nullptr)
            , mDataManager(nullptr)
            , mLogManager(nullptr)
            , mLogFacility(nullptr)
        {
            mLogManager = new MyGUI::LogManager();
            mRenderManager = new RenderManager(viewer, guiRoot, textureManager, uiScalingFactor);
            mDataManager = new DataManager();
        }

        ~Platform()
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

        void initialise(const std::string& resourcePath, const std::string& _logName = "MyGUI.log")
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

        void shutdown()
        {
            mRenderManager->shutdown();
            mDataManager->shutdown();
        }

        RenderManager* getRenderManagerPtr()
        {
            return mRenderManager;
        }

        DataManager* getDataManagerPtr()
        {
            return mDataManager;
        }

    private:
        RenderManager* mRenderManager;
        DataManager* mDataManager;
        MyGUI::LogManager* mLogManager;
        LogFacility* mLogFacility;
    };

}

#endif
