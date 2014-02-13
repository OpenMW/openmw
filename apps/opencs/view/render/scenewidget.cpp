#include "scenewidget.hpp"

#include <QEvent>
#include <QResizeEvent>

#include <OgreRoot.h>
#include <OgreRenderWindow.h>
#include <OgreEntity.h>
#include <OgreCamera.h>

namespace CSVRender
{

    SceneWidget::SceneWidget(QWidget *parent)
        : QWidget(parent)
        , mWindow(NULL)
        , mCamera(NULL)
        , mSceneMgr(NULL)
    {
        setAttribute(Qt::WA_PaintOnScreen);
        setAttribute(Qt::WA_NoSystemBackground);

        mSceneMgr = Ogre::Root::getSingleton().createSceneManager(Ogre::ST_GENERIC);

        // Throw in a random color just to make sure multiple scenes work
        Ogre::Real r = Ogre::Math::RangeRandom(0, 1);
        Ogre::Real g = Ogre::Math::RangeRandom(0, 1);
        Ogre::Real b = Ogre::Math::RangeRandom(0, 1);
        mSceneMgr->setAmbientLight(Ogre::ColourValue(r,g,b,1));

        Ogre::Light* l = mSceneMgr->createLight();
        l->setType (Ogre::Light::LT_DIRECTIONAL);
        l->setDirection (Ogre::Vector3(-0.4, -0.7, 0.3));
        l->setDiffuseColour (Ogre::ColourValue(0.7,0.7,0.7));

        mCamera = mSceneMgr->createCamera("foo");

        Ogre::Entity* ent = mSceneMgr->createEntity("cube", Ogre::SceneManager::PT_CUBE);
        ent->setMaterialName("BaseWhite");

        mSceneMgr->getRootSceneNode()->attachObject(ent);

        mCamera->setPosition(300,300,300);
        mCamera->lookAt(0,0,0);
        mCamera->setNearClipDistance(0.1);
        mCamera->setFarClipDistance(3000);
    }

    void SceneWidget::updateOgreWindow()
    {
        if (mWindow)
        {
            Ogre::Root::getSingleton().destroyRenderTarget(mWindow);
            mWindow = NULL;
        }

        std::stringstream windowHandle;
        windowHandle << this->winId();

        std::stringstream windowTitle;
        static int count=0;
        windowTitle << ++count;

        Ogre::NameValuePairList params;

        params.insert(std::make_pair("externalWindowHandle",  windowHandle.str()));
        params.insert(std::make_pair("title", windowTitle.str()));
        params.insert(std::make_pair("FSAA", "0")); // TODO setting
        params.insert(std::make_pair("vsync", "false")); // TODO setting

        mWindow = Ogre::Root::getSingleton().createRenderWindow(windowTitle.str(), this->width(), this->height(), false, &params);
        mWindow->addViewport(mCamera)->setBackgroundColour(Ogre::ColourValue(0.3,0.3,0.3,1));

        Ogre::Real aspectRatio = Ogre::Real(width()) / Ogre::Real(height());
        mCamera->setAspectRatio(aspectRatio);
    }

    SceneWidget::~SceneWidget()
    {
        Ogre::Root::getSingleton().destroyRenderTarget(mWindow);
    }

    void SceneWidget::paintEvent(QPaintEvent* e)
    {
        if (!mWindow)
            updateOgreWindow();

        mWindow->update();
        e->accept();
    }


    QPaintEngine* SceneWidget::paintEngine() const
    {
        // We don't want another paint engine to get in the way.
        // So we return nothing.
        return NULL;
    }

    void SceneWidget::resizeEvent(QResizeEvent *e)
    {
        if (!mWindow)
            return;

        const QSize &newSize = e->size();

        // TODO: Fix Ogre to handle this more consistently (fixed in 1.9)
#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
        mWindow->resize(newSize.width(), newSize.height());
#else
        mWindow->windowMovedOrResized();
#endif

        Ogre::Real aspectRatio = Ogre::Real(newSize.width()) / Ogre::Real(newSize.height());
        mCamera->setAspectRatio(aspectRatio);
    }

    bool SceneWidget::event(QEvent *e)
    {
        if (e->type() == QEvent::WinIdChange)
        {
            // I haven't actually seen this happen yet.
            if (mWindow)
                updateOgreWindow();
        }
        return QWidget::event(e);
    }

}
