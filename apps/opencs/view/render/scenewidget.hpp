#ifndef OPENCS_VIEW_SCENEWIDGET_H
#define OPENCS_VIEW_SCENEWIDGET_H

#include <QWidget>

#include <OgreColourValue.h>

#include "lightingday.hpp"
#include "lightingnight.hpp"
#include "lightingbright.hpp"

namespace Ogre
{
    class Camera;
    class SceneManager;
    class RenderWindow;
    class Viewport;
}

namespace CSVWidget
{
    class SceneToolMode;
    class SceneToolbar;
}

namespace CSVRender
{
    class Navigation;
    class Lighting;

    class SceneWidget : public QWidget
    {
        Q_OBJECT

        public:

            SceneWidget(QWidget *parent);
            virtual ~SceneWidget();

            QPaintEngine* paintEngine() const;

            CSVWidget::SceneToolMode *makeLightingSelector (CSVWidget::SceneToolbar *parent);
            ///< \attention The created tool is not added to the toolbar (via addTool). Doing that
            /// is the responsibility of the calling function.

            virtual void setVisibilityMask (unsigned int mask);

        protected:

            void setNavigation (Navigation *navigation);
            ///< \attention The ownership of \a navigation is not transferred to *this.

            Ogre::SceneManager *getSceneManager();

            Ogre::Camera *getCamera();

            void flagAsModified();

            void setDefaultAmbient (const Ogre::ColourValue& colour);
            ///< \note The actual ambient colour may differ based on lighting settings.

        private:
            void paintEvent(QPaintEvent* e);
            void resizeEvent(QResizeEvent* e);
            bool event(QEvent* e);

            void keyPressEvent (QKeyEvent *event);

            void keyReleaseEvent (QKeyEvent *event);

            void focusOutEvent (QFocusEvent *event);

            void wheelEvent (QWheelEvent *event);

            void leaveEvent (QEvent *event);

            void mouseMoveEvent (QMouseEvent *event);

            void mouseReleaseEvent (QMouseEvent *event);

            void updateOgreWindow();

            int getFastFactor() const;

            void setLighting (Lighting *lighting);
            ///< \attention The ownership of \a lighting is not transferred to *this.

            Ogre::Camera*	    mCamera;
            Ogre::SceneManager* mSceneMgr;
            Ogre::RenderWindow* mWindow;
            Ogre::Viewport *mViewport;

            Navigation *mNavigation;
            Lighting *mLighting;
            bool mUpdate;
            bool mKeyForward;
            bool mKeyBackward;
            bool mKeyLeft;
            bool mKeyRight;
            bool mKeyRollLeft;
            bool mKeyRollRight;
            bool mFast;
            bool mDragging;
            bool mMod1;
            QPoint mOldPos;
            int mFastFactor;
            Ogre::ColourValue mDefaultAmbient;
            bool mHasDefaultAmbient;
            LightingDay mLightingDay;
            LightingNight mLightingNight;
            LightingBright mLightingBright;

        private slots:

            void update();

            void selectLightingMode (const std::string& mode);

        signals:

            void focusToolbarRequest();
    };
}

#endif
