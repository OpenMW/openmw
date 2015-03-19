#ifndef OPENCS_VIEW_SCENEWIDGET_H
#define OPENCS_VIEW_SCENEWIDGET_H

#include <QWidget>
#include <QTimer>

#include "lightingday.hpp"
#include "lightingnight.hpp"
#include "lightingbright.hpp"

#include <osgViewer/Viewer>

namespace osg
{
    class Group;
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

    class SceneWidget : public QWidget, public osgViewer::Viewer
    {
        Q_OBJECT

    public:
        SceneWidget(QWidget* parent = 0, Qt::WindowFlags f = 0);

        virtual void paintEvent( QPaintEvent* event );

        void flagAsModified();

    protected:

        osg::Group* mRootNode;

        QTimer mTimer;
    };

}

#endif
