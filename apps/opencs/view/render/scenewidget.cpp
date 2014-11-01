#include "scenewidget.hpp"

#include <QEvent>
#include <QResizeEvent>
#include <QTimer>
#include <QShortcut>

#include <OgreRoot.h>
#include <OgreRenderWindow.h>
#include <OgreEntity.h>
#include <OgreCamera.h>
#include <OgreSceneNode.h>
#include <OgreViewport.h>
#include <OgreOverlaySystem.h>

#include "../widget/scenetoolmode.hpp"
#include "../../model/settings/usersettings.hpp"

#include "navigation.hpp"
#include "lighting.hpp"
#include "overlaysystem.hpp"

namespace CSVRender
{
    SceneWidget::SceneWidget(QWidget *parent)
        : QWidget(parent)
        , mCamera(NULL)
        , mSceneMgr(NULL)
        , mWindow(NULL)
        , mViewport(NULL)
        , mNavigation (0), mLighting (0), mUpdate (false), mKeyForward (false)
        , mKeyBackward (false), mKeyLeft (false), mKeyRight (false)
        , mKeyRollLeft (false), mKeyRollRight (false)
        , mFast (false), mDragging (false), mMod1 (false)
        , mFastFactor (4)
        , mDefaultAmbient (0, 0, 0, 0), mHasDefaultAmbient (false)
    {
        setAttribute(Qt::WA_PaintOnScreen);
        setAttribute(Qt::WA_NoSystemBackground);

        setFocusPolicy (Qt::StrongFocus);

        mSceneMgr = Ogre::Root::getSingleton().createSceneManager(Ogre::ST_GENERIC);

        mSceneMgr->setAmbientLight (Ogre::ColourValue (0,0,0,1));

        mCamera = mSceneMgr->createCamera("foo");

        mCamera->setPosition (300, 0, 0);
        mCamera->lookAt (0, 0, 0);
        mCamera->setNearClipDistance (0.1);

        CSMSettings::UserSettings &userSettings = CSMSettings::UserSettings::instance();

        float farClipDist = userSettings.setting("3d-render/far-clip-distance", QString("300000")).toFloat();
        mCamera->setFarClipDistance (farClipDist);

        mFastFactor = userSettings.setting("scene-input/fast-factor", QString("4")).toInt();

        mCamera->roll (Ogre::Degree (90));

        setLighting (&mLightingDay);

        mOverlaySystem = OverlaySystem::instance().get();
        mSceneMgr->addRenderQueueListener(mOverlaySystem);

        QTimer *timer = new QTimer (this);

        connect (timer, SIGNAL (timeout()), this, SLOT (update()));

        int timerStart = userSettings.setting("scene-input/timer", QString("20")).toInt();
        timer->start (timerStart);

        /// \todo make shortcut configurable
        QShortcut *focusToolbar = new QShortcut (Qt::Key_T, this, 0, 0, Qt::WidgetWithChildrenShortcut);
        connect (focusToolbar, SIGNAL (activated()), this, SIGNAL (focusToolbarRequest()));
    }

    CSVWidget::SceneToolMode *SceneWidget::makeLightingSelector (CSVWidget::SceneToolbar *parent)
    {
        CSVWidget::SceneToolMode *tool = new CSVWidget::SceneToolMode (parent, "Lighting Mode");

        /// \todo replace icons
        tool->addButton (":scenetoolbar/day", "day",
            "Day"
            "<ul><li>Cell specific ambient in interiors</li>"
            "<li>Low ambient in exteriors</li>"
            "<li>Strong directional light source/lir>"
            "<li>This mode closely resembles day time in-game</li></ul>");
        tool->addButton (":scenetoolbar/night", "night",
            "Night"
            "<ul><li>Cell specific ambient in interiors</li>"
            "<li>Low ambient in exteriors</li>"
            "<li>Weak directional light source</li>"
            "<li>This mode closely resembles night time in-game</li></ul>");
        tool->addButton (":scenetoolbar/bright", "bright",
            "Bright"
            "<ul><li>Maximum ambient</li>"
            "<li>Strong directional light source</li></ul>");

        connect (tool, SIGNAL (modeChanged (const std::string&)),
            this, SLOT (selectLightingMode (const std::string&)));

        return tool;
    }

    void SceneWidget::setDefaultAmbient (const Ogre::ColourValue& colour)
    {
        mDefaultAmbient = colour;
        mHasDefaultAmbient = true;

        if (mLighting)
            mLighting->setDefaultAmbient (colour);
    }

    void SceneWidget::updateOgreWindow()
    {
        if (mWindow)
        {
            Ogre::Root::getSingleton().destroyRenderTarget(mWindow);
            mWindow = NULL;
        }

        std::stringstream windowHandle;
#ifdef WIN32
        windowHandle << Ogre::StringConverter::toString((uintptr_t)(this->winId()));
#else
        windowHandle << this->winId();
#endif
        std::stringstream windowTitle;
        static int count=0;
        windowTitle << ++count;

        Ogre::NameValuePairList params;

        params.insert(std::make_pair("externalWindowHandle",  windowHandle.str()));
        params.insert(std::make_pair("title", windowTitle.str()));

        std::string antialiasing =
            CSMSettings::UserSettings::instance().settingValue("3d-render/antialiasing").toStdString();
        if(antialiasing == "MSAA 16")     antialiasing = "16";
        else if(antialiasing == "MSAA 8") antialiasing = "8";
        else if(antialiasing == "MSAA 4") antialiasing = "4";
        else if(antialiasing == "MSAA 2") antialiasing = "2";
        else                              antialiasing = "0";
        params.insert(std::make_pair("FSAA", antialiasing));

        params.insert(std::make_pair("vsync", "false")); // TODO setting
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
        params.insert(std::make_pair("macAPI", "cocoa"));
        params.insert(std::make_pair("macAPICocoaUseNSView", "true"));
#endif

        mWindow = Ogre::Root::getSingleton().createRenderWindow(windowTitle.str(), this->width(), this->height(), false, &params);

        mViewport = mWindow->addViewport (mCamera);
        mViewport->setBackgroundColour (Ogre::ColourValue (0.3,0.3,0.3,1));

        Ogre::Real aspectRatio = Ogre::Real(width()) / Ogre::Real(height());
        mCamera->setAspectRatio(aspectRatio);
    }

    SceneWidget::~SceneWidget()
    {
        if (mWindow)
            Ogre::Root::getSingleton().destroyRenderTarget (mWindow);

        if (mSceneMgr)
            mSceneMgr->removeRenderQueueListener (mOverlaySystem);

        if (mSceneMgr)
            Ogre::Root::getSingleton().destroySceneManager (mSceneMgr);

    }

    void SceneWidget::setVisibilityMask (unsigned int mask)
    {
        mViewport->setVisibilityMask (mask);
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

    void SceneWidget::addRenderTargetListener(Ogre::RenderTargetListener *listener)
    {
        mWindow->addListener(listener);
    }

    void SceneWidget::removeRenderTargetListener(Ogre::RenderTargetListener *listener)
    {
        mWindow->removeListener(listener);
    }

    Ogre::Viewport *SceneWidget::getViewport()
    {
        if (!mWindow)
            updateOgreWindow();

        return mViewport;
    }

    Ogre::SceneManager *SceneWidget::getSceneManager()
    {
        return mSceneMgr;
    }

    Ogre::Camera *SceneWidget::getCamera()
    {
        return mCamera;
    }

    void SceneWidget::flagAsModified()
    {
        mUpdate = true;
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

        if (mUpdate && mWindow)
        {
            mUpdate = false;
            mWindow->update();
            updateOverlay();
        }
    }

    void SceneWidget::updateScene()
    {
        flagAsModified();
    }

    void SceneWidget::updateOverlay()
    { }

    void SceneWidget::setLighting (Lighting *lighting)
    {
        if (mLighting)
            mLighting->deactivate();

        mLighting = lighting;
        mLighting->activate (mSceneMgr, mHasDefaultAmbient ? &mDefaultAmbient : 0);

        if (mWindow)
            mWindow->update();
    }

    void SceneWidget::selectLightingMode (const std::string& mode)
    {
        if (mode=="day")
            setLighting (&mLightingDay);
        else if (mode=="night")
            setLighting (&mLightingNight);
        else if (mode=="bright")
            setLighting (&mLightingBright);
    }

    void SceneWidget::updateUserSetting (const QString &key, const QStringList &list)
    {
        if(key.contains(QRegExp("^\\b(Objects|Shader|Scene)", Qt::CaseInsensitive)))
            flagAsModified();

        if(key == "3d-render/far-clip-distance" && !list.empty())
        {
            if(mCamera->getFarClipDistance() != list.at(0).toFloat())
                mCamera->setFarClipDistance(list.at(0).toFloat());
        }

        // minimise unnecessary ogre window creation by updating only when there is a change
        if(key == "3d-render/antialiasing")
        {
            unsigned int aa = mWindow->getFSAA();
            unsigned int antialiasing = 0;
            if(!list.empty())
            {
                if(list.at(0) == "MSAA 16")     antialiasing = 16;
                else if(list.at(0) == "MSAA 8") antialiasing = 8;
                else if(list.at(0) == "MSAA 4") antialiasing = 4;
                else if(list.at(0) == "MSAA 2") antialiasing = 2;
            }
            if(aa != antialiasing)
                updateOgreWindow();
        }
    }
}
