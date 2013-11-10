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
        SceneWidget(QWidget *parent);
        virtual ~SceneWidget(void);

        QPaintEngine*	paintEngine() const;

    private:
        void paintEvent(QPaintEvent* e);
        void resizeEvent(QResizeEvent* e);
        bool event(QEvent* e);

        void updateOgreWindow();

        Ogre::Camera*	    mCamera;
        Ogre::SceneManager* mSceneMgr;
        Ogre::RenderWindow* mWindow;
    };

}

#endif
