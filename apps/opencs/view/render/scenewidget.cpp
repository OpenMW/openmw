#include "scenewidget.hpp"

#include <QEvent>
#include <QResizeEvent>
#include <QTimer>
#include <QShortcut>
#include <QLayout>

#include <osgQt/GraphicsWindowQt>
#include <osg/GraphicsContext>
#include <osgViewer/CompositeViewer>
#include <osgViewer/ViewerEventHandlers>
#include <osg/LightModel>

#include <components/resource/scenemanager.hpp>
#include <components/resource/resourcesystem.hpp>

#include "../widget/scenetoolmode.hpp"
#include "../../model/settings/usersettings.hpp"

#include "lighting.hpp"

namespace CSVRender
{

RenderWidget::RenderWidget(QWidget *parent, Qt::WindowFlags f)
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
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(window->getGLWidget());
    setLayout(layout);

    // Pass events through this widget first
    window->getGLWidget()->installEventFilter(this);

    mView->getCamera()->setGraphicsContext(window);
    mView->getCamera()->setClearColor( osg::Vec4(0.2, 0.2, 0.6, 1.0) );
    mView->getCamera()->setViewport( new osg::Viewport(0, 0, traits->width, traits->height) );
    mView->getCamera()->setProjectionMatrixAsPerspective(30.0f, static_cast<double>(traits->width)/static_cast<double>(traits->height), 1.0f, 10000.0f );

    mRootNode = new osg::Group;

    mView->getCamera()->getOrCreateStateSet()->setMode(GL_NORMALIZE, osg::StateAttribute::ON);
    mView->getCamera()->getOrCreateStateSet()->setMode(GL_CULL_FACE, osg::StateAttribute::ON);

    mView->setSceneData(mRootNode);

    // Press S to reveal profiling stats
    mView->addEventHandler(new osgViewer::StatsHandler);

    mView->getCamera()->setCullMask(~(0x1));

    viewer.addView(mView);
    viewer.setDone(false);
    viewer.realize();
}

RenderWidget::~RenderWidget()
{
    CompositeViewer::get().removeView(mView);
}

void RenderWidget::flagAsModified()
{
    mView->requestRedraw();
}

void RenderWidget::setVisibilityMask(int mask)
{
    // 0x1 reserved for separating cull and update visitors
    mView->getCamera()->setCullMask(mask<<1);
}

bool RenderWidget::eventFilter(QObject* obj, QEvent* event)
{
    // handle event in this widget, is there a better way to do this?
    if (event->type() == QEvent::MouseButtonPress)
        mousePressEvent(static_cast<QMouseEvent*>(event));
    if (event->type() == QEvent::MouseButtonRelease)
        mouseReleaseEvent(static_cast<QMouseEvent*>(event));
    if (event->type() == QEvent::MouseMove)
        mouseMoveEvent(static_cast<QMouseEvent*>(event));
    if (event->type() == QEvent::KeyPress)
        keyPressEvent(static_cast<QKeyEvent*>(event));
    if (event->type() == QEvent::KeyRelease)
        keyReleaseEvent(static_cast<QKeyEvent*>(event));
    if (event->type() == QEvent::Wheel)
        wheelEvent(static_cast<QWheelEvent *>(event));

    // Always pass the event on to GLWidget, i.e. to OSG event queue
    return QObject::eventFilter(obj, event);
}

// --------------------------------------------------

CompositeViewer::CompositeViewer()
    : mSimulationTime(0.0)
{
#if QT_VERSION >= 0x050000
    // Qt5 is currently crashing and reporting "Cannot make QOpenGLContext current in a different thread" when the viewer is run multi-threaded, this is regression from Qt4
    osgViewer::ViewerBase::ThreadingModel threadingModel = osgViewer::ViewerBase::SingleThreaded;
#else
    osgViewer::ViewerBase::ThreadingModel threadingModel = osgViewer::ViewerBase::DrawThreadPerContext;
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
    mSimulationTime += mFrameTimer.time_s();
    mFrameTimer.setStartTick();
    frame(mSimulationTime);
}

// ---------------------------------------------------

SceneWidget::SceneWidget(boost::shared_ptr<Resource::ResourceSystem> resourceSystem, QWidget *parent, Qt::WindowFlags f)
    : RenderWidget(parent, f)
    , mResourceSystem(resourceSystem)
    , mLighting(NULL)
    , mHasDefaultAmbient(false)
{
    // we handle lighting manually
    mView->setLightingMode(osgViewer::View::NO_LIGHT);

    setLighting(&mLightingDay);

    /// \todo make shortcut configurable
    QShortcut *focusToolbar = new QShortcut (Qt::Key_T, this, 0, 0, Qt::WidgetWithChildrenShortcut);
    connect (focusToolbar, SIGNAL (activated()), this, SIGNAL (focusToolbarRequest()));
}

SceneWidget::~SceneWidget()
{
    // Since we're holding on to the scene templates past the existance of this graphics context, we'll need to manually release the created objects
    mResourceSystem->getSceneManager()->releaseGLObjects(mView->getCamera()->getGraphicsContext()->getState());
}

void SceneWidget::setLighting(Lighting *lighting)
{
    if (mLighting)
        mLighting->deactivate();

    mLighting = lighting;
    mLighting->activate (mRootNode);

    osg::Vec4f ambient = mLighting->getAmbientColour(mHasDefaultAmbient ? &mDefaultAmbient : 0);
    setAmbient(ambient);

    flagAsModified();
}

void SceneWidget::setAmbient(const osg::Vec4f& ambient)
{
    osg::ref_ptr<osg::StateSet> stateset = new osg::StateSet;
    osg::ref_ptr<osg::LightModel> lightmodel = new osg::LightModel;
    lightmodel->setAmbientIntensity(ambient);
    stateset->setMode(GL_LIGHTING, osg::StateAttribute::ON);
    stateset->setMode(GL_LIGHT0, osg::StateAttribute::ON);
    stateset->setAttributeAndModes(lightmodel, osg::StateAttribute::ON);
    mRootNode->setStateSet(stateset);
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

CSVWidget::SceneToolMode *SceneWidget::makeLightingSelector (CSVWidget::SceneToolbar *parent)
{
    CSVWidget::SceneToolMode *tool = new CSVWidget::SceneToolMode (parent, "Lighting Mode");

    /// \todo replace icons
    tool->addButton (":scenetoolbar/day", "day",
        "Day"
        "<ul><li>Cell specific ambient in interiors</li>"
        "<li>Low ambient in exteriors</li>"
        "<li>Strong directional light source</li>"
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

void SceneWidget::setDefaultAmbient (const osg::Vec4f& colour)
{
    mDefaultAmbient = colour;
    mHasDefaultAmbient = true;

    setAmbient(mLighting->getAmbientColour(&mDefaultAmbient));
}

}
