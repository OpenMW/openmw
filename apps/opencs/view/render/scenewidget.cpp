#include "scenewidget.hpp"

#include <QEvent>
#include <QResizeEvent>
#include <QTimer>
#include <QLayout>

#include <extern/osgQt/GraphicsWindowQt>
#include <osg/GraphicsContext>
#include <osgViewer/CompositeViewer>
#include <osgViewer/ViewerEventHandlers>
#include <osg/LightModel>
#include <osg/Material>
#include <osg/Version>

#include <components/debug/debuglog.hpp>
#include <components/resource/scenemanager.hpp>
#include <components/resource/resourcesystem.hpp>
#include <components/sceneutil/lightmanager.hpp>

#include "../widget/scenetoolmode.hpp"

#include "../../model/prefs/state.hpp"
#include "../../model/prefs/shortcut.hpp"
#include "../../model/prefs/shortcuteventhandler.hpp"

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
    updateCameraParameters( traits->width / static_cast<double>(traits->height) );

    osg::ref_ptr<osgQt::GraphicsWindowQt> window = new osgQt::GraphicsWindowQt(traits.get());
    QLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(window->getGLWidget());
    setLayout(layout);

    mView->getCamera()->setGraphicsContext(window);
    mView->getCamera()->setClearColor( osg::Vec4(0.2, 0.2, 0.6, 1.0) );
    mView->getCamera()->setViewport( new osg::Viewport(0, 0, traits->width, traits->height) );

    SceneUtil::LightManager* lightMgr = new SceneUtil::LightManager;
    lightMgr->setStartLight(1);
    lightMgr->setLightingMask(Mask_Lighting);
    mRootNode = lightMgr;

    mView->getCamera()->getOrCreateStateSet()->setMode(GL_NORMALIZE, osg::StateAttribute::ON);
    mView->getCamera()->getOrCreateStateSet()->setMode(GL_CULL_FACE, osg::StateAttribute::ON);
    osg::ref_ptr<osg::Material> defaultMat (new osg::Material);
    defaultMat->setColorMode(osg::Material::OFF);
    defaultMat->setAmbient(osg::Material::FRONT_AND_BACK, osg::Vec4f(1,1,1,1));
    defaultMat->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4f(1,1,1,1));
    defaultMat->setSpecular(osg::Material::FRONT_AND_BACK, osg::Vec4f(0.f, 0.f, 0.f, 0.f));
    mView->getCamera()->getOrCreateStateSet()->setAttribute(defaultMat);

    mView->setSceneData(mRootNode);

    // Add ability to signal osg to show its statistics for debugging purposes
    mView->addEventHandler(new osgViewer::StatsHandler);

    mView->getCamera()->setCullMask(~(Mask_UpdateVisitor));

    viewer.addView(mView);
    viewer.setDone(false);
    viewer.realize();
}

RenderWidget::~RenderWidget()
{
    try
    {
        CompositeViewer::get().removeView(mView);

#if OSG_VERSION_LESS_THAN(3,6,5)
        // before OSG 3.6.4, the default font was a static object, and if it wasn't attached to the scene when a graphics context was destroyed, it's program wouldn't be released.
        // 3.6.4 moved it into the object cache, which meant it usually got released, but not here.
        // 3.6.5 improved cleanup with osgViewer::CompositeViewer::removeView so it more reliably released associated state for objects in the object cache.
        osg::ref_ptr<osg::GraphicsContext> graphicsContext = mView->getCamera()->getGraphicsContext();
        osgText::Font::getDefaultFont()->releaseGLObjects(graphicsContext->getState());
#endif
    }
    catch(const std::exception& e)
    {
        Log(Debug::Error) << "Error in the destructor: " << e.what();
    }
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

void RenderWidget::toggleRenderStats()
{
    osgViewer::GraphicsWindow* window =
        static_cast<osgViewer::GraphicsWindow*>(mView->getCamera()->getGraphicsContext());

    window->getEventQueue()->keyPress(osgGA::GUIEventAdapter::KEY_S);
    window->getEventQueue()->keyRelease(osgGA::GUIEventAdapter::KEY_S);
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

#if OSG_VERSION_GREATER_OR_EQUAL(3,5,5)
    setUseConfigureAffinity(false);
#endif

    // disable the default setting of viewer.done() by pressing Escape.
    setKeyEventSetsDone(0);

    // Only render when the camera position changed, or content flagged dirty
    //setRunFrameScheme(osgViewer::ViewerBase::ON_DEMAND);
    setRunFrameScheme(osgViewer::ViewerBase::CONTINUOUS);

    connect( &mTimer, SIGNAL(timeout()), this, SLOT(update()) );
    mTimer.start( 10 );

    int frameRateLimit = CSMPrefs::get()["Rendering"]["framerate-limit"].toInt();
    setRunMaxFrameRate(frameRateLimit);
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

    double minFrameTime = _runMaxFrameRate > 0.0 ? 1.0 / _runMaxFrameRate : 0.0;
    if (dt < minFrameTime)
    {
        OpenThreads::Thread::microSleep(1000*1000*(minFrameTime-dt));
    }
}

// ---------------------------------------------------

SceneWidget::SceneWidget(std::shared_ptr<Resource::ResourceSystem> resourceSystem, QWidget *parent, Qt::WindowFlags f,
    bool retrieveInput)
    : RenderWidget(parent, f)
    , mResourceSystem(resourceSystem)
    , mLighting(nullptr)
    , mHasDefaultAmbient(false)
    , mIsExterior(true)
    , mPrevMouseX(0)
    , mPrevMouseY(0)
    , mCamPositionSet(false)
{
    mFreeCamControl = new FreeCameraController(this);
    mOrbitCamControl = new OrbitCameraController(this);
    mCurrentCamControl = mFreeCamControl;

    mOrbitCamControl->setPickingMask(Mask_Reference | Mask_Terrain);

    mOrbitCamControl->setConstRoll( CSMPrefs::get()["3D Scene Input"]["navi-orbit-const-roll"].isTrue() );

    // we handle lighting manually
    mView->setLightingMode(osgViewer::View::NO_LIGHT);

    setLighting(&mLightingDay);

    mResourceSystem->getSceneManager()->setParticleSystemMask(Mask_ParticleSystem);

    // Recieve mouse move event even if mouse button is not pressed
    setMouseTracking(true);
    setFocusPolicy(Qt::ClickFocus);

    connect (&CSMPrefs::State::get(), SIGNAL (settingChanged (const CSMPrefs::Setting *)),
        this, SLOT (settingChanged (const CSMPrefs::Setting *)));

    // TODO update this outside of the constructor where virtual methods can be used
    if (retrieveInput)
    {
        CSMPrefs::get()["3D Scene Input"].update();
        CSMPrefs::get()["Tooltips"].update();
    }

    connect (&CompositeViewer::get(), SIGNAL (simulationUpdated(double)), this, SLOT (update(double)));

    // Shortcuts
    CSMPrefs::Shortcut* focusToolbarShortcut = new CSMPrefs::Shortcut("scene-focus-toolbar", this);
    connect(focusToolbarShortcut, SIGNAL(activated()), this, SIGNAL(focusToolbarRequest()));

    CSMPrefs::Shortcut* renderStatsShortcut = new CSMPrefs::Shortcut("scene-render-stats", this);
    connect(renderStatsShortcut, SIGNAL(activated()), this, SLOT(toggleRenderStats()));
}

SceneWidget::~SceneWidget()
{
    // Since we're holding on to the resources past the existence of this graphics context, we'll need to manually release the created objects
    mResourceSystem->releaseGLObjects(mView->getCamera()->getGraphicsContext()->getState());
}

void SceneWidget::setLighting(Lighting *lighting)
{
    if (mLighting)
        mLighting->deactivate();

    mLighting = lighting;
    mLighting->activate (mRootNode, mIsExterior);

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

void SceneWidget::setExterior (bool isExterior)
{
    mIsExterior = isExterior;
}

void SceneWidget::mouseMoveEvent (QMouseEvent *event)
{
    mCurrentCamControl->handleMouseMoveEvent(event->x() - mPrevMouseX, event->y() - mPrevMouseY);

    mPrevMouseX = event->x();
    mPrevMouseY = event->y();
}

void SceneWidget::wheelEvent(QWheelEvent *event)
{
    mCurrentCamControl->handleMouseScrollEvent(event->delta());
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
    else if (*setting=="3D Scene Input/navi-orbit-const-roll")
    {
        mOrbitCamControl->setConstRoll(setting->isTrue());
    }
    else if (*setting=="Rendering/framerate-limit")
    {
        CompositeViewer::get().setRunMaxFrameRate(setting->toInt());
    }
    else if (*setting=="Rendering/camera-fov" ||
             *setting=="Rendering/camera-ortho" ||
             *setting=="Rendering/camera-ortho-size")
    {
        updateCameraParameters();
    }
}

void RenderWidget::updateCameraParameters(double overrideAspect)
{
    const float nearDist = 1.0;
    const float farDist = 1000.0;

    if (CSMPrefs::get()["Rendering"]["camera-ortho"].isTrue())
    {
        const float size = CSMPrefs::get()["Rendering"]["camera-ortho-size"].toInt();
        const float aspect = overrideAspect >= 0.0 ? overrideAspect : (width() / static_cast<double>(height()));
        const float halfH = size * 10.0;
        const float halfW = halfH * aspect;

        mView->getCamera()->setProjectionMatrixAsOrtho(
            -halfW, halfW, -halfH, halfH, nearDist, farDist);
    }
    else
    { 
        mView->getCamera()->setProjectionMatrixAsPerspective(
            CSMPrefs::get()["Rendering"]["camera-fov"].toInt(),
            static_cast<double>(width())/static_cast<double>(height()),
            nearDist, farDist);
    }
}

void SceneWidget::selectNavigationMode (const std::string& mode)
{
    if (mode=="1st")
    {
        mCurrentCamControl->setCamera(nullptr);
        mCurrentCamControl = mFreeCamControl;
        mFreeCamControl->setCamera(getCamera());
        mFreeCamControl->fixUpAxis(CameraController::WorldUp);
    }
    else if (mode=="free")
    {
        mCurrentCamControl->setCamera(nullptr);
        mCurrentCamControl = mFreeCamControl;
        mFreeCamControl->setCamera(getCamera());
        mFreeCamControl->unfixUpAxis();
    }
    else if (mode=="orbit")
    {
        mCurrentCamControl->setCamera(nullptr);
        mCurrentCamControl = mOrbitCamControl;
        mOrbitCamControl->setCamera(getCamera());
        mOrbitCamControl->reset();
    }
}

}
