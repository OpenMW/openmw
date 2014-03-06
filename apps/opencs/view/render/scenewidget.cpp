#include "scenewidget.hpp"

#include <QEvent>
#include <QResizeEvent>
#include <QTimer>

#include <OgreRoot.h>
#include <OgreRenderWindow.h>
#include <OgreEntity.h>
#include <OgreCamera.h>
#include <OgreSceneNode.h>
#include <OgreViewport.h>

#include "navigation.hpp"

namespace CSVRender
{
    SceneWidget::SceneWidget(QWidget *parent)
        : QWidget(parent)
        , mWindow(NULL)
        , mCamera(NULL)
        , mSceneMgr(NULL), mNavigation (0), mUpdate (false)
        , mKeyForward (false), mKeyBackward (false), mKeyLeft (false), mKeyRight (false)
        , mKeyRollLeft (false), mKeyRollRight (false)
        , mFast (false), mDragging (false), mMod1 (false)
        , mFastFactor (4) /// \todo make this configurable
    {
        setAttribute(Qt::WA_PaintOnScreen);
        setAttribute(Qt::WA_NoSystemBackground);

        setFocusPolicy (Qt::StrongFocus);

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

        QTimer *timer = new QTimer (this);

        connect (timer, SIGNAL (timeout()), this, SLOT (update()));
        timer->start (20); /// \todo make this configurable
    }

    void SceneWidget::setAmbient (const Ogre::ColourValue& colour)
    {
        mSceneMgr->setAmbientLight (colour);
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
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
        params.insert(std::make_pair("macAPI", "cocoa"));
        params.insert(std::make_pair("macAPICocoaUseNSView", "true"));
#endif

        mWindow = Ogre::Root::getSingleton().createRenderWindow(windowTitle.str(), this->width(), this->height(), false, &params);
        mWindow->addViewport(mCamera)->setBackgroundColour(Ogre::ColourValue(0.3,0.3,0.3,1));

        Ogre::Real aspectRatio = Ogre::Real(width()) / Ogre::Real(height());
        mCamera->setAspectRatio(aspectRatio);
    }

    SceneWidget::~SceneWidget()
    {
        if (mWindow)
            Ogre::Root::getSingleton().destroyRenderTarget (mWindow);

        if (mSceneMgr)
            Ogre::Root::getSingleton().destroySceneManager (mSceneMgr);
    }

    void SceneWidget::setNavigation (Navigation *navigation)
    {
        if ((mNavigation = navigation))
        {
            mNavigation->setFastModeFactor (mFast ? mFastFactor : 1);
            if (mNavigation->activate (mCamera))
                mUpdate = true;
        }
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

    void SceneWidget::keyPressEvent (QKeyEvent *event)
    {
        switch (event->key())
        {
            case Qt::Key_W: mKeyForward = true; break;
            case Qt::Key_S: mKeyBackward = true; break;
            case Qt::Key_A: mKeyLeft = true; break;
            case Qt::Key_D: mKeyRight = true; break;
            case Qt::Key_Q: mKeyRollLeft = true; break;
            case Qt::Key_E: mKeyRollRight = true; break;
            case Qt::Key_Control: mMod1 = true; break;

            case Qt::Key_Shift:

                mFast = true;

                if (mNavigation)
                    mNavigation->setFastModeFactor (mFastFactor);

                break;

            default: QWidget::keyPressEvent (event);
        }
    }

    void SceneWidget::keyReleaseEvent (QKeyEvent *event)
    {
        switch (event->key())
        {
            case Qt::Key_W: mKeyForward = false; break;
            case Qt::Key_S: mKeyBackward = false; break;
            case Qt::Key_A: mKeyLeft = false; break;
            case Qt::Key_D: mKeyRight = false; break;
            case Qt::Key_Q: mKeyRollLeft = false; break;
            case Qt::Key_E: mKeyRollRight = false; break;
            case Qt::Key_Control: mMod1 = false; break;

            case Qt::Key_Shift:

                mFast = false;

                if (mNavigation)
                    mNavigation->setFastModeFactor (1);

                break;

            default: QWidget::keyReleaseEvent (event);
        }
    }

    void SceneWidget::wheelEvent (QWheelEvent *event)
    {
        if (mNavigation)
            if (event->delta())
                if (mNavigation->wheelMoved (event->delta()))
                    mUpdate = true;
    }

    void SceneWidget::leaveEvent (QEvent *event)
    {
        mDragging = false;
    }

    void SceneWidget::mouseMoveEvent (QMouseEvent *event)
    {
        if (event->buttons() & Qt::LeftButton)
        {
            if (mDragging)
            {
                QPoint diff = mOldPos-event->pos();
                mOldPos = event->pos();

                if (mNavigation)
                    if (mNavigation->mouseMoved (diff, mMod1 ? 1 : 0))
                        mUpdate = true;
            }
            else
            {
                mDragging = true;
                mOldPos = event->pos();
            }
        }
    }

    void SceneWidget::mouseReleaseEvent (QMouseEvent *event)
    {
        if (!(event->buttons() & Qt::LeftButton))
            mDragging = false;
    }

    void SceneWidget::focusOutEvent (QFocusEvent *event)
    {
        mKeyForward = false;
        mKeyBackward = false;
        mKeyLeft = false;
        mKeyRight = false;
        mFast = false;
        mMod1 = false;

        QWidget::focusOutEvent (event);
    }

    void SceneWidget::update()
    {
        if (mNavigation)
        {
            int horizontal = 0;
            int vertical = 0;

            if (mKeyForward && !mKeyBackward)
                vertical = 1;
            else if (!mKeyForward && mKeyBackward)
                vertical = -1;

            if (mKeyLeft && !mKeyRight)
                horizontal = -1;
            else if (!mKeyLeft && mKeyRight)
                horizontal = 1;

            if (horizontal || vertical)
                if (mNavigation->handleMovementKeys (vertical, horizontal))
                    mUpdate = true;

            int roll = 0;

            if (mKeyRollLeft && !mKeyRollRight)
                roll = 1;
            else if (!mKeyRollLeft && mKeyRollRight)
                roll = -1;

            if (roll)
                if (mNavigation->handleRollKeys (roll))
                    mUpdate = true;

        }

        if (mUpdate)
        {
            mUpdate = false;
            mWindow->update();
        }
    }

    int SceneWidget::getFastFactor() const
    {
        return mFast ? mFastFactor : 1;
    }
}
