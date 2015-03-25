#include "scenewidget.hpp"

#include <QEvent>
#include <QResizeEvent>
#include <QTimer>
#include <QShortcut>
#include <QLayout>

#include "../widget/scenetoolmode.hpp"
#include "../../model/settings/usersettings.hpp"

#include "navigation.hpp"
#include "lighting.hpp"

#include <osgQt/GraphicsWindowQt>
#include <osg/GraphicsContext>
#include <osgViewer/CompositeViewer>
#include <osgViewer/ViewerEventHandlers>

namespace CSVRender
{

SceneWidget::SceneWidget(QWidget *parent, Qt::WindowFlags f)
    : QWidget(parent, f)
    , mRootNode(0)
{

    osgViewer::CompositeViewer& viewer = CompositeViewer::get();

    osg::DisplaySettings* ds = osg::DisplaySettings::instance().get();
    //ds->setNumMultiSamples(8);

    osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits;
    traits->windowName = "";
    traits->windowDecoration = true;
    traits->x = 0;
    traits->y = 0;
    traits->width = width();
    traits->height = height();
    traits->doubleBuffer = true;
    traits->alpha = ds->getMinimumNumAlphaBits();
    traits->stencil = ds->getMinimumNumStencilBits();
    traits->sampleBuffers = ds->getMultiSamples();
    traits->samples = ds->getNumMultiSamples();
    // Doesn't make much sense as we're running on demand updates, and there seems to be a bug with the refresh rate when running multiple QGLWidgets
    traits->vsync = false;

    mView = new osgViewer::View;

    osg::ref_ptr<osgQt::GraphicsWindowQt> window = new osgQt::GraphicsWindowQt(traits.get());
    QLayout* layout = new QHBoxLayout(this);
    layout->addWidget(window->getGLWidget());
    setLayout(layout);

    mView->getCamera()->setGraphicsContext(window);
    mView->getCamera()->setClearColor( osg::Vec4(0.2, 0.2, 0.6, 1.0) );
    mView->getCamera()->setViewport( new osg::Viewport(0, 0, traits->width, traits->height) );
    mView->getCamera()->setProjectionMatrixAsPerspective(30.0f, static_cast<double>(traits->width)/static_cast<double>(traits->height), 1.0f, 10000.0f );

    mRootNode = new osg::Group;
    // TODO: move to utility file
    mRootNode->getOrCreateStateSet()->setMode(GL_NORMALIZE, osg::StateAttribute::ON);
    mRootNode->getOrCreateStateSet()->setMode(GL_CULL_FACE, osg::StateAttribute::ON);

    mView->setSceneData(mRootNode);

    // Press S to reveal profiling stats
    mView->addEventHandler(new osgViewer::StatsHandler);

    mView->getCamera()->setCullMask(~(0x1));

    viewer.addView(mView);
    viewer.setDone(false);
    viewer.realize();
}

SceneWidget::~SceneWidget()
{
    CompositeViewer::get().removeView(mView);
}

void SceneWidget::flagAsModified()
{
    mView->requestRedraw();
}

CompositeViewer::CompositeViewer()
{
#if QT_VERSION >= 0x050000
    // Qt5 is currently crashing and reporting "Cannot make QOpenGLContext current in a different thread" when the viewer is run multi-threaded, this is regression from Qt4
    osgViewer::ViewerBase::ThreadingModel threadingModel = osgViewer::ViewerBase::SingleThreaded;
#else
    osgViewer::ViewerBase::ThreadingModel threadingModel = osgViewer::ViewerBase::CullDrawThreadPerContext;
#endif

    setThreadingModel(threadingModel);

    // disable the default setting of viewer.done() by pressing Escape.
    setKeyEventSetsDone(0);

    // Only render when the camera position changed, or content flagged dirty
    //setRunFrameScheme(osgViewer::ViewerBase::ON_DEMAND);
    setRunFrameScheme(osgViewer::ViewerBase::CONTINUOUS);

    connect( &mTimer, SIGNAL(timeout()), this, SLOT(update()) );
    mTimer.start( 10 );
}

CompositeViewer &CompositeViewer::get()
{
    static CompositeViewer sThis;
    return sThis;
}

void CompositeViewer::update()
{
    frame();
}

}
