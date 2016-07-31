#include "scenewidget.hpp"

#include <QEvent>
#include <QResizeEvent>
#include <QTimer>
#include <QShortcut>
#include <QLayout>

#include <extern/osgQt/GraphicsWindowQt>
#include <osg/GraphicsContext>
#include <osgViewer/CompositeViewer>
#include <osgViewer/ViewerEventHandlers>
#include <osg/LightModel>

#include <components/resource/scenemanager.hpp>
#include <components/resource/resourcesystem.hpp>
#include <components/sceneutil/lightmanager.hpp>

#include "../widget/scenetoolmode.hpp"

#include "../../model/prefs/state.hpp"

#include "lighting.hpp"
#include "mask.hpp"
#include "cameracontroller.hpp"

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

    mView->getCamera()->setGraphicsContext(window);
    mView->getCamera()->setClearColor( osg::Vec4(0.2, 0.2, 0.6, 1.0) );
    mView->getCamera()->setViewport( new osg::Viewport(0, 0, traits->width, traits->height) );
    mView->getCamera()->setProjectionMatrixAsPerspective(30.0f, static_cast<double>(traits->width)/static_cast<double>(traits->height), 1.0f, 10000.0f );

    SceneUtil::LightManager* lightMgr = new SceneUtil::LightManager;
    lightMgr->setStartLight(1);
    lightMgr->setLightingMask(Mask_Lighting);
    mRootNode = lightMgr;

    mView->getCamera()->getOrCreateStateSet()->setMode(GL_NORMALIZE, osg::StateAttribute::ON);
    mView->getCamera()->getOrCreateStateSet()->setMode(GL_CULL_FACE, osg::StateAttribute::ON);

    mView->setSceneData(mRootNode);

    // Press S to reveal profiling stats
    mView->addEventHandler(new osgViewer::StatsHandler);

    mView->getCamera()->setCullMask(~(Mask_UpdateVisitor));

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
    mView->getCamera()->setCullMask(mask | Mask_ParticleSystem | Mask_Lighting);
}

osg::Camera *RenderWidget::getCamera()
{
    return mView->getCamera();
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
    double dt = mFrameTimer.time_s();
    mFrameTimer.setStartTick();

    emit simulationUpdated(dt);

    mSimulationTime += dt;
    frame(mSimulationTime);
}

// ---------------------------------------------------

SceneWidget::SceneWidget(boost::shared_ptr<Resource::ResourceSystem> resourceSystem, QWidget *parent, Qt::WindowFlags f,
    bool retrieveInput)
    : RenderWidget(parent, f)
    , mResourceSystem(resourceSystem)
    , mLighting(NULL)
    , mHasDefaultAmbient(false)
    , mPrevMouseX(0)
    , mPrevMouseY(0)
    , mFreeCamControl(new FreeCameraController())
    , mOrbitCamControl(new OrbitCameraController())
    , mCurrentCamControl(mFreeCamControl.get())
    , mCamPositionSet(false)
{
    mOrbitCamControl->setPickingMask(Mask_Reference | Mask_Terrain);
    selectNavigationMode("free");

    // we handle lighting manually
    mView->setLightingMode(osgViewer::View::NO_LIGHT);

    setLighting(&mLightingDay);

    mResourceSystem->getSceneManager()->setParticleSystemMask(Mask_ParticleSystem);

    // Recieve mouse move event even if mouse button is not pressed
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);

    /// \todo make shortcut configurable
    QShortcut *focusToolbar = new QShortcut (Qt::Key_T, this, 0, 0, Qt::WidgetWithChildrenShortcut);
    connect (focusToolbar, SIGNAL (activated()), this, SIGNAL (focusToolbarRequest()));

    connect (&CSMPrefs::State::get(), SIGNAL (settingChanged (const CSMPrefs::Setting *)),
        this, SLOT (settingChanged (const CSMPrefs::Setting *)));

    // TODO update this outside of the constructor where virtual methods can be used
    if (retrieveInput)
    {
        CSMPrefs::get()["3D Scene Input"].update();
        CSMPrefs::get()["Tooltips"].update();
    }

    connect (&CompositeViewer::get(), SIGNAL (simulationUpdated(double)), this, SLOT (update(double)));
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

void SceneWidget::mousePressEvent (QMouseEvent *event)
{
    mMouseMode = mapButton(event);

    mPrevMouseX = event->x();
    mPrevMouseY = event->y();
}

void SceneWidget::mouseReleaseEvent (QMouseEvent *event)
{
    mMouseMode = "";
}

void SceneWidget::mouseMoveEvent (QMouseEvent *event)
{
    mCurrentCamControl->handleMouseMoveEvent(mMouseMode, event->x() - mPrevMouseX, event->y() - mPrevMouseY);

    mPrevMouseX = event->x();
    mPrevMouseY = event->y();
}

void SceneWidget::focusOutEvent (QFocusEvent *event)
{
    mCurrentCamControl->resetInput();
}

void SceneWidget::wheelEvent(QWheelEvent *event)
{
    mCurrentCamControl->handleMouseMoveEvent("t-navi", event->delta(), 0);
}

void SceneWidget::keyPressEvent (QKeyEvent *event)
{
    mCurrentCamControl->handleKeyEvent(event, true);
}

void SceneWidget::keyReleaseEvent (QKeyEvent *event)
{
    mCurrentCamControl->handleKeyEvent(event, false);
}

void SceneWidget::update(double dt)
{
    if (mCamPositionSet)
    {
        mCurrentCamControl->update(dt);
    }
    else
    {
        mCurrentCamControl->setup(mRootNode, Mask_Reference | Mask_Terrain, CameraController::WorldUp);
        mCamPositionSet = true;
    }
}

void SceneWidget::settingChanged (const CSMPrefs::Setting *setting)
{
    if (*setting=="3D Scene Input/p-navi-free-sensitivity")
    {
        mFreeCamControl->setCameraSensitivity(setting->toDouble());
    }
    else if (*setting=="3D Scene Input/p-navi-orbit-sensitivity")
    {
        mOrbitCamControl->setCameraSensitivity(setting->toDouble());
    }
    else if (*setting=="3D Scene Input/p-navi-free-invert")
    {
        mFreeCamControl->setInverted(setting->isTrue());
    }
    else if (*setting=="3D Scene Input/p-navi-orbit-invert")
    {
        mOrbitCamControl->setInverted(setting->isTrue());
    }
    else if (*setting=="3D Scene Input/s-navi-sensitivity")
    {
        mFreeCamControl->setSecondaryMovementMultiplier(setting->toDouble());
        mOrbitCamControl->setSecondaryMovementMultiplier(setting->toDouble());
    }
    else if (*setting=="3D Scene Input/navi-wheel-factor")
    {
        mFreeCamControl->setWheelMovementMultiplier(setting->toDouble());
        mOrbitCamControl->setWheelMovementMultiplier(setting->toDouble());
    }
    else if (*setting=="3D Scene Input/navi-free-lin-speed")
    {
        mFreeCamControl->setLinearSpeed(setting->toDouble());
    }
    else if (*setting=="3D Scene Input/navi-free-rot-speed")
    {
        mFreeCamControl->setRotationalSpeed(setting->toDouble());
    }
    else if (*setting=="3D Scene Input/navi-free-speed-mult")
    {
        mFreeCamControl->setSpeedMultiplier(setting->toDouble());
    }
    else if (*setting=="3D Scene Input/navi-orbit-rot-speed")
    {
        mOrbitCamControl->setOrbitSpeed(setting->toDouble());
    }
    else if (*setting=="3D Scene Input/navi-orbit-speed-mult")
    {
        mOrbitCamControl->setOrbitSpeedMultiplier(setting->toDouble());
    }
    else
    {
        storeMappingSetting(setting);
    }
}

void SceneWidget::selectNavigationMode (const std::string& mode)
{
    if (mode=="1st")
    {
        mCurrentCamControl->setCamera(NULL);
        mCurrentCamControl = mFreeCamControl.get();
        mCurrentCamControl->setCamera(getCamera());
        mFreeCamControl->fixUpAxis(CameraController::WorldUp);
    }
    else if (mode=="free")
    {
        mCurrentCamControl->setCamera(NULL);
        mCurrentCamControl = mFreeCamControl.get();
        mCurrentCamControl->setCamera(getCamera());
        mFreeCamControl->unfixUpAxis();
    }
    else if (mode=="orbit")
    {
        mCurrentCamControl->setCamera(NULL);
        mCurrentCamControl = mOrbitCamControl.get();
        mCurrentCamControl->setCamera(getCamera());
    }
}

bool SceneWidget::storeMappingSetting (const CSMPrefs::Setting *setting)
{
    if (setting->getParent()->getKey()!="3D Scene Input")
        return false;

    static const char * const sMappingSettings[] =
    {
        "p-navi", "s-navi",
        0
    };

    for (int i=0; sMappingSettings[i]; ++i)
        if (setting->getKey()==sMappingSettings[i])
        {
            QString value = QString::fromUtf8 (setting->toString().c_str());

            Qt::MouseButton button = Qt::NoButton;

            if (value.endsWith ("Left Mouse-Button"))
                button = Qt::LeftButton;
            else if (value.endsWith ("Right Mouse-Button"))
                button = Qt::RightButton;
            else if (value.endsWith ("Middle Mouse-Button"))
                button = Qt::MiddleButton;
            else
                return false;

            bool ctrl = value.startsWith ("Ctrl-");

            mButtonMapping[std::make_pair (button, ctrl)] = sMappingSettings[i];
            return true;
        }

    return false;
}

std::string SceneWidget::mapButton (QMouseEvent *event)
{
    std::pair<Qt::MouseButton, bool> phyiscal (
        event->button(), event->modifiers() & Qt::ControlModifier);

    std::map<std::pair<Qt::MouseButton, bool>, std::string>::const_iterator iter =
        mButtonMapping.find (phyiscal);

    if (iter!=mButtonMapping.end())
        return iter->second;

    return "";
}

}
