#ifndef OPENMW_MWGUI_MYGUIPLATFORM_H
#define OPENMW_MWGUI_MYGUIPLATFORM_H

#include "MyGUI_Prerequest.h"
#include "MyGUI_DummyRenderManager.h"
#include "MyGUI_DummyDataManager.h"
#include "MyGUI_DummyDiagnostic.h"
#include "MyGUI_LogManager.h"

#include "myguirendermanager.hpp"
#include "myguidatamanager.hpp"

namespace MWGui
{

    class Platform
    {
    public:
        Platform(osgViewer::Viewer* viewer, osg::Group* guiRoot, Resource::TextureManager* textureManager) :
            mLogManager(nullptr),
            mRenderManager(nullptr),
            mDataManager(nullptr)
        {
            mLogManager = new MyGUI::LogManager();
            mRenderManager = new RenderManager(viewer, guiRoot, textureManager);
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
        }

        void initialise(const std::string& resourcePath, const std::string& _logName = MYGUI_PLATFORM_LOG_FILENAME)
        {
            if (!_logName.empty())
                MyGUI::LogManager::getInstance().createDefaultSource(_logName);

            mDataManager->setResourcePath(resourcePath);

            mRenderManager->initialise();
            mDataManager->initialise();
        }

        void shutdown()
        {
            //mRenderManager->shutdown();
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
    };

}

#endif
