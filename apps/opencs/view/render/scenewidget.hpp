#ifndef OPENCS_VIEW_SCENEWIDGET_H
#define OPENCS_VIEW_SCENEWIDGET_H

#include <QWidget>

namespace Ogre
{
    class Camera;
    class SceneManager;
    class RenderWindow;
}

namespace CSVRender
{
    class SceneWidget : public QWidget
    {
        Q_OBJECT

        public:

            enum NavigationMode
            {
                NavigationMode_Free
            };

            SceneWidget(QWidget *parent);
            virtual ~SceneWidget(void);

            QPaintEngine*	paintEngine() const;

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

            Ogre::Camera*	    mCamera;
            Ogre::SceneManager* mSceneMgr;
            Ogre::RenderWindow* mWindow;

            NavigationMode mNavigationMode;
            bool mUpdate;
            int mKeyForward;
            int mKeyBackward;
            int mKeyLeft;
            int mKeyRight;
            bool mFast;
            bool mDragging;
            QPoint mOldPos;
            int mMouseSensitivity;
            int mFastFactor;

        private slots:

            void update();
    };
}

#endif
