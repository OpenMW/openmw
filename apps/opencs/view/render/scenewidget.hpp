#ifndef OPENCS_VIEW_SCENEWIDGET_H
#define OPENCS_VIEW_SCENEWIDGET_H

#include <QWidget>
#include <QTimer>

#include "lightingday.hpp"
#include "lightingnight.hpp"
#include "lightingbright.hpp"

#include <osgViewer/View>
#include <osgViewer/CompositeViewer>

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

    class SceneWidget : public QWidget
    {
        Q_OBJECT

    public:
        SceneWidget(QWidget* parent = 0, Qt::WindowFlags f = 0);
        ~SceneWidget();

        void flagAsModified();

    protected:

        osg::ref_ptr<osgViewer::View> mView;

        osg::Group* mRootNode;

        QTimer mTimer;
    };



    // There are rendering glitches when using multiple Viewer instances, work around using CompositeViewer with multiple views
    class CompositeViewer : public QObject, public osgViewer::CompositeViewer
    {
        Q_OBJECT
    public:
        CompositeViewer();

        static CompositeViewer& get();

        QTimer mTimer;

    public slots:
        void update();
    };

}

#endif
