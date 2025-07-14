#include "scenewidget.hpp"

#include <chrono>
#include <exception>
#include <thread>

#include <QLayout>
#include <QMouseEvent>
#include <QSurfaceFormat>

#include <apps/opencs/model/prefs/category.hpp>
#include <apps/opencs/model/prefs/setting.hpp>
#include <apps/opencs/view/render/lightingbright.hpp>
#include <apps/opencs/view/render/lightingday.hpp>
#include <apps/opencs/view/render/lightingnight.hpp>

#include <extern/osgQt/CompositeOsgRenderer.hpp>
#include <extern/osgQt/osgQOpenGLWidget.hpp>

#include <osg/Array>
#include <osg/Camera>
#include <osg/DisplaySettings>
#include <osg/GL>
#include <osg/Geometry>
#include <osg/GraphicsContext>
#include <osg/Group>
#include <osg/LightModel>
#include <osg/Material>
#include <osg/Matrix>
#include <osg/PrimitiveSet>
#include <osg/StateAttribute>
#include <osg/StateSet>
#include <osg/Transform>
#include <osg/Vec3>
#include <osg/Vec4>
#include <osg/Vec4ub>
#include <osg/View>
#include <osg/Viewport>

#include <osgGA/EventQueue>
#include <osgGA/GUIEventAdapter>

#include <osgViewer/GraphicsWindow>
#include <osgViewer/View>
#include <osgViewer/ViewerBase>
#include <osgViewer/ViewerEventHandlers>

#include <components/debug/debuglog.hpp>
#include <components/resource/resourcesystem.hpp>
#include <components/resource/scenemanager.hpp>
#include <components/sceneutil/glextensions.hpp>
#include <components/sceneutil/lightmanager.hpp>

#include "../widget/scenetoolmode.hpp"

#include "../../model/prefs/shortcut.hpp"
#include "../../model/prefs/state.hpp"

#include "cameracontroller.hpp"
#include "lighting.hpp"
#include "mask.hpp"

namespace CSVRender
{

    RenderWidget::RenderWidget(QWidget* parent, Qt::WindowFlags f)
        : QWidget(parent, f)
        , mRootNode(nullptr)
    {
        mView = new osgViewer::View;
        updateCameraParameters(width() / static_cast<double>(height()));

        mWidget = new osgQOpenGLWidget(this);

        mRenderer = mWidget->getCompositeViewer();
        osg::ref_ptr<osgViewer::GraphicsWindowEmbedded> window
            = new osgViewer::GraphicsWindowEmbedded(0, 0, width(), height());
        mWidget->setGraphicsWindowEmbedded(window);

        mRenderer->setRealizeOperation(new SceneUtil::GetGLExtensionsOperation());

        int frameRateLimit = CSMPrefs::get()["Rendering"]["framerate-limit"].toInt();
        mRenderer->setRunMaxFrameRate(frameRateLimit);
        mRenderer->setUseConfigureAffinity(false);

        QLayout* layout = new QHBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(mWidget);
        setLayout(layout);

        mView->getCamera()->setGraphicsContext(window);

        osg::ref_ptr<SceneUtil::LightManager> lightMgr = new SceneUtil::LightManager;
        lightMgr->setStartLight(1);
        lightMgr->setLightingMask(Mask_Lighting);
        mRootNode = std::move(lightMgr);

        mView->getCamera()->setViewport(new osg::Viewport(0, 0, width(), height()));

        mView->getCamera()->getOrCreateStateSet()->setMode(GL_NORMALIZE, osg::StateAttribute::ON);
        mView->getCamera()->getOrCreateStateSet()->setMode(GL_CULL_FACE, osg::StateAttribute::ON);
        osg::ref_ptr<osg::Material> defaultMat(new osg::Material);
        defaultMat->setColorMode(osg::Material::OFF);
        defaultMat->setAmbient(osg::Material::FRONT_AND_BACK, osg::Vec4f(1, 1, 1, 1));
        defaultMat->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4f(1, 1, 1, 1));
        defaultMat->setSpecular(osg::Material::FRONT_AND_BACK, osg::Vec4f(0.f, 0.f, 0.f, 0.f));
        mView->getCamera()->getOrCreateStateSet()->setAttribute(defaultMat);

        mView->setSceneData(mRootNode);

        // Add ability to signal osg to show its statistics for debugging purposes
        mView->addEventHandler(new osgViewer::StatsHandler);

        mRenderer->addView(mView);
        mRenderer->setDone(false);
    }

    RenderWidget::~RenderWidget()
    {
        try
        {
            mRenderer->removeView(mView);
        }
        catch (const std::exception& e)
        {
            Log(Debug::Error) << "Error in the destructor: " << e.what();
        }
        delete mWidget;
    }

    void RenderWidget::flagAsModified()
    {
        mView->requestRedraw();
    }

    void RenderWidget::setVisibilityMask(unsigned int mask)
    {
        mView->getCamera()->setCullMask(mask | Mask_ParticleSystem | Mask_Lighting);
    }

    osg::Camera* RenderWidget::getCamera()
    {
        return mView->getCamera();
    }

    void RenderWidget::toggleRenderStats()
    {
        osgViewer::GraphicsWindow* window
            = static_cast<osgViewer::GraphicsWindow*>(mView->getCamera()->getGraphicsContext());

        window->getEventQueue()->keyPress(osgGA::GUIEventAdapter::KEY_S);
        window->getEventQueue()->keyRelease(osgGA::GUIEventAdapter::KEY_S);
    }

    // ---------------------------------------------------

    SceneWidget::SceneWidget(std::shared_ptr<Resource::ResourceSystem> resourceSystem, QWidget* parent,
        Qt::WindowFlags f, bool retrieveInput)
        : RenderWidget(parent, f)
        , mResourceSystem(std::move(resourceSystem))
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

        mOrbitCamControl->setConstRoll(CSMPrefs::get()["3D Scene Input"]["navi-orbit-const-roll"].isTrue());

        // set up gradient view or configured clear color
        QColor bgColour = CSMPrefs::get()["Rendering"]["scene-day-background-colour"].toColor();

        if (CSMPrefs::get()["Rendering"]["scene-use-gradient"].isTrue())
        {
            QColor gradientColour = CSMPrefs::get()["Rendering"]["scene-day-gradient-colour"].toColor();
            mGradientCamera = createGradientCamera(bgColour, gradientColour);

            mView->getCamera()->setClearMask(0);
            mView->getCamera()->addChild(mGradientCamera.get());
        }
        else
        {
            mView->getCamera()->setClearColor(osg::Vec4(bgColour.redF(), bgColour.greenF(), bgColour.blueF(), 1.0f));
        }

        // we handle lighting manually
        mView->setLightingMode(osgViewer::View::NO_LIGHT);

        setLighting(&mLightingDay);

        mResourceSystem->getSceneManager()->setParticleSystemMask(Mask_ParticleSystem);

        // Recieve mouse move event even if mouse button is not pressed
        setMouseTracking(true);
        setFocusPolicy(Qt::ClickFocus);

        connect(&CSMPrefs::State::get(), &CSMPrefs::State::settingChanged, this, &SceneWidget::settingChanged);

        // TODO update this outside of the constructor where virtual methods can be used
        if (retrieveInput)
        {
            CSMPrefs::get()["3D Scene Input"].update();
            CSMPrefs::get()["Tooltips"].update();
        }

        connect(mRenderer, &CompositeOsgRenderer::simulationUpdated, this, &SceneWidget::update);

        // Shortcuts
        CSMPrefs::Shortcut* focusToolbarShortcut = new CSMPrefs::Shortcut("scene-focus-toolbar", this);
        connect(
            focusToolbarShortcut, qOverload<>(&CSMPrefs::Shortcut::activated), this, &SceneWidget::focusToolbarRequest);

        CSMPrefs::Shortcut* renderStatsShortcut = new CSMPrefs::Shortcut("scene-render-stats", this);
        connect(
            renderStatsShortcut, qOverload<>(&CSMPrefs::Shortcut::activated), this, &SceneWidget::toggleRenderStats);
    }

    SceneWidget::~SceneWidget()
    {
        // Since we're holding on to the resources past the existence of this graphics context, we'll need to manually
        // release the created objects
        mResourceSystem->releaseGLObjects(mView->getCamera()->getGraphicsContext()->getState());
    }

    osg::ref_ptr<osg::Geometry> SceneWidget::createGradientRectangle(QColor& bgColour, QColor& gradientColour)
    {
        osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry;

        osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;

        vertices->push_back(osg::Vec3(0.0f, 0.0f, -1.0f));
        vertices->push_back(osg::Vec3(1.0f, 0.0f, -1.0f));
        vertices->push_back(osg::Vec3(0.0f, 1.0f, -1.0f));
        vertices->push_back(osg::Vec3(1.0f, 1.0f, -1.0f));

        geometry->setVertexArray(vertices);

        osg::ref_ptr<osg::DrawElementsUShort> primitives = new osg::DrawElementsUShort(osg::PrimitiveSet::TRIANGLES, 0);

        // triangle 1
        primitives->push_back(0);
        primitives->push_back(1);
        primitives->push_back(2);

        // triangle 2
        primitives->push_back(2);
        primitives->push_back(1);
        primitives->push_back(3);

        geometry->addPrimitiveSet(primitives);

        osg::ref_ptr<osg::Vec4ubArray> colours = new osg::Vec4ubArray;
        colours->push_back(osg::Vec4ub(gradientColour.red(), gradientColour.green(), gradientColour.blue(), 1.0f));
        colours->push_back(osg::Vec4ub(gradientColour.red(), gradientColour.green(), gradientColour.blue(), 1.0f));
        colours->push_back(osg::Vec4ub(bgColour.red(), bgColour.green(), bgColour.blue(), 1.0f));
        colours->push_back(osg::Vec4ub(bgColour.red(), bgColour.green(), bgColour.blue(), 1.0f));

        geometry->setColorArray(colours, osg::Array::BIND_PER_VERTEX);

        geometry->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
        geometry->getOrCreateStateSet()->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);

        return geometry;
    }

    osg::ref_ptr<osg::Camera> SceneWidget::createGradientCamera(QColor& bgColour, QColor& gradientColour)
    {
        osg::ref_ptr<osg::Camera> camera = new osg::Camera();
        camera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
        camera->setProjectionMatrix(osg::Matrix::ortho2D(0, 1.0f, 0, 1.0f));
        camera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
        camera->setViewMatrix(osg::Matrix::identity());

        camera->setClearMask(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
        camera->setAllowEventFocus(false);

        // draw subgraph before main camera view.
        camera->setRenderOrder(osg::Camera::PRE_RENDER);

        camera->getOrCreateStateSet()->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);

        osg::ref_ptr<osg::Geometry> gradientQuad = createGradientRectangle(bgColour, gradientColour);

        camera->addChild(std::move(gradientQuad));
        return camera;
    }

    void SceneWidget::updateGradientCamera(QColor& bgColour, QColor& gradientColour)
    {
        osg::ref_ptr<osg::Geometry> gradientRect = createGradientRectangle(bgColour, gradientColour);
        // Replaces previous rectangle
        mGradientCamera->setChild(0, gradientRect.get());
    }

    void SceneWidget::setLighting(Lighting* lighting)
    {
        if (mLighting)
            mLighting->deactivate();

        mLighting = lighting;
        mLighting->activate(mRootNode, mIsExterior);

        osg::Vec4f ambient = mLighting->getAmbientColour(mHasDefaultAmbient ? &mDefaultAmbient : nullptr);
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

    void SceneWidget::selectLightingMode(const std::string& mode)
    {
        QColor backgroundColour;
        QColor gradientColour;
        if (mode == "day")
        {
            backgroundColour = CSMPrefs::get()["Rendering"]["scene-day-background-colour"].toColor();
            gradientColour = CSMPrefs::get()["Rendering"]["scene-day-gradient-colour"].toColor();
            setLighting(&mLightingDay);
        }
        else if (mode == "night")
        {
            backgroundColour = CSMPrefs::get()["Rendering"]["scene-night-background-colour"].toColor();
            gradientColour = CSMPrefs::get()["Rendering"]["scene-night-gradient-colour"].toColor();
            setLighting(&mLightingNight);
        }
        else if (mode == "bright")
        {
            backgroundColour = CSMPrefs::get()["Rendering"]["scene-bright-background-colour"].toColor();
            gradientColour = CSMPrefs::get()["Rendering"]["scene-bright-gradient-colour"].toColor();
            setLighting(&mLightingBright);
        }
        if (CSMPrefs::get()["Rendering"]["scene-use-gradient"].isTrue())
        {
            if (mGradientCamera.get() != nullptr)
            {
                // we can go ahead and update since this camera still exists
                updateGradientCamera(backgroundColour, gradientColour);

                if (!mView->getCamera()->containsNode(mGradientCamera.get()))
                {
                    // need to re-attach the gradient camera
                    mView->getCamera()->setClearMask(0);
                    mView->getCamera()->addChild(mGradientCamera.get());
                }
            }
            else
            {
                // need to create the gradient camera
                mGradientCamera = createGradientCamera(backgroundColour, gradientColour);
                mView->getCamera()->setClearMask(0);
                mView->getCamera()->addChild(mGradientCamera.get());
            }
        }
        else
        {
            // Fall back to using the clear color for the camera
            mView->getCamera()->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            mView->getCamera()->setClearColor(
                osg::Vec4(backgroundColour.redF(), backgroundColour.greenF(), backgroundColour.blueF(), 1.0f));
            if (mGradientCamera.get() != nullptr && mView->getCamera()->containsNode(mGradientCamera.get()))
            {
                // Remove the child to prevent the gradient from rendering
                mView->getCamera()->removeChild(mGradientCamera.get());
            }
        }
    }

    CSVWidget::SceneToolMode* SceneWidget::makeLightingSelector(CSVWidget::SceneToolbar* parent)
    {
        CSVWidget::SceneToolMode* tool = new CSVWidget::SceneToolMode(parent, "Lighting Mode");

        /// \todo replace icons
        tool->addButton(":scenetoolbar/day", "day",
            "Day"
            "<ul><li>Cell specific ambient in interiors</li>"
            "<li>Low ambient in exteriors</li>"
            "<li>Strong directional light source</li>"
            "<li>This mode closely resembles day time in-game</li></ul>");
        tool->addButton(":scenetoolbar/night", "night",
            "Night"
            "<ul><li>Cell specific ambient in interiors</li>"
            "<li>Low ambient in exteriors</li>"
            "<li>Weak directional light source</li>"
            "<li>This mode closely resembles night time in-game</li></ul>");
        tool->addButton(":scenetoolbar/bright", "bright",
            "Bright"
            "<ul><li>Maximum ambient</li>"
            "<li>Strong directional light source</li></ul>");

        connect(tool, &CSVWidget::SceneToolMode::modeChanged, this, &SceneWidget::selectLightingMode);

        return tool;
    }

    void SceneWidget::setDefaultAmbient(const osg::Vec4f& colour)
    {
        mDefaultAmbient = colour;
        mHasDefaultAmbient = true;

        setAmbient(mLighting->getAmbientColour(&mDefaultAmbient));
    }

    void SceneWidget::setExterior(bool isExterior)
    {
        mIsExterior = isExterior;
    }

    void SceneWidget::mouseMoveEvent(QMouseEvent* event)
    {
        mCurrentCamControl->handleMouseMoveEvent(event->x() - mPrevMouseX, event->y() - mPrevMouseY);

        mPrevMouseX = event->x();
        mPrevMouseY = event->y();
    }

    void SceneWidget::wheelEvent(QWheelEvent* event)
    {
        mCurrentCamControl->handleMouseScrollEvent(event->angleDelta().y());
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

        if (mSelectionMarkerNode)
        {
            osg::MatrixList worldMats = mSelectionMarkerNode->getWorldMatrices();
            if (!worldMats.empty())
            {
                osg::Matrixd markerWorldMat = worldMats[0];

                osg::Vec3f eye, _;
                mView->getCamera()->getViewMatrix().getLookAt(eye, _, _);
                osg::Vec3f cameraLocalPos = eye * osg::Matrixd::inverse(markerWorldMat);

                bool isInFrontRightQuadrant = (cameraLocalPos.x() > 0.1f) && (cameraLocalPos.y() > 0.1f);
                bool isSignificantlyBehind = (cameraLocalPos.x() < 1.f) && (cameraLocalPos.y() < 1.f);

                if (!isInFrontRightQuadrant && isSignificantlyBehind)
                {
                    osg::Quat current = mSelectionMarkerNode->getAttitude();
                    mSelectionMarkerNode->setAttitude(current * osg::Quat(osg::PI, osg::Vec3f(0, 0, 1)));
                }

                float distance = (markerWorldMat.getTrans() - eye).length();
                float scale = std::max(distance / 75.0f, 1.0f);
                mSelectionMarkerNode->setScale(osg::Vec3(scale, scale, scale));
            }
        }
    }

    void SceneWidget::settingChanged(const CSMPrefs::Setting* setting)
    {
        if (*setting == "3D Scene Input/p-navi-free-sensitivity")
        {
            mFreeCamControl->setCameraSensitivity(setting->toDouble());
        }
        else if (*setting == "3D Scene Input/p-navi-orbit-sensitivity")
        {
            mOrbitCamControl->setCameraSensitivity(setting->toDouble());
        }
        else if (*setting == "3D Scene Input/p-navi-free-invert")
        {
            mFreeCamControl->setInverted(setting->isTrue());
        }
        else if (*setting == "3D Scene Input/p-navi-orbit-invert")
        {
            mOrbitCamControl->setInverted(setting->isTrue());
        }
        else if (*setting == "3D Scene Input/s-navi-sensitivity")
        {
            mFreeCamControl->setSecondaryMovementMultiplier(setting->toDouble());
            mOrbitCamControl->setSecondaryMovementMultiplier(setting->toDouble());
        }
        else if (*setting == "3D Scene Input/navi-wheel-factor")
        {
            mFreeCamControl->setWheelMovementMultiplier(setting->toDouble());
            mOrbitCamControl->setWheelMovementMultiplier(setting->toDouble());
        }
        else if (*setting == "3D Scene Input/navi-free-lin-speed")
        {
            mFreeCamControl->setLinearSpeed(setting->toDouble());
        }
        else if (*setting == "3D Scene Input/navi-free-rot-speed")
        {
            mFreeCamControl->setRotationalSpeed(setting->toDouble());
        }
        else if (*setting == "3D Scene Input/navi-free-speed-mult")
        {
            mFreeCamControl->setSpeedMultiplier(setting->toDouble());
        }
        else if (*setting == "3D Scene Input/navi-orbit-rot-speed")
        {
            mOrbitCamControl->setOrbitSpeed(setting->toDouble());
        }
        else if (*setting == "3D Scene Input/navi-orbit-speed-mult")
        {
            mOrbitCamControl->setOrbitSpeedMultiplier(setting->toDouble());
        }
        else if (*setting == "3D Scene Input/navi-orbit-const-roll")
        {
            mOrbitCamControl->setConstRoll(setting->isTrue());
        }
        else if (*setting == "Rendering/framerate-limit")
        {
            mRenderer->setRunMaxFrameRate(setting->toInt());
        }
        else if (*setting == "Rendering/camera-fov" || *setting == "Rendering/camera-ortho"
            || *setting == "Rendering/camera-ortho-size")
        {
            updateCameraParameters();
        }
        else if (*setting == "Rendering/scene-day-night-switch-nodes")
        {
            if (mLighting)
                setLighting(mLighting);
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

            mView->getCamera()->setProjectionMatrixAsOrtho(-halfW, halfW, -halfH, halfH, nearDist, farDist);
        }
        else
        {
            mView->getCamera()->setProjectionMatrixAsPerspective(CSMPrefs::get()["Rendering"]["camera-fov"].toInt(),
                static_cast<double>(width()) / static_cast<double>(height()), nearDist, farDist);
        }
    }

    void SceneWidget::selectNavigationMode(const std::string& mode)
    {
        if (mode == "1st")
        {
            mCurrentCamControl->setCamera(nullptr);
            mCurrentCamControl = mFreeCamControl;
            mFreeCamControl->setCamera(getCamera());
            mFreeCamControl->fixUpAxis(CameraController::WorldUp);
        }
        else if (mode == "free")
        {
            mCurrentCamControl->setCamera(nullptr);
            mCurrentCamControl = mFreeCamControl;
            mFreeCamControl->setCamera(getCamera());
            mFreeCamControl->unfixUpAxis();
        }
        else if (mode == "orbit")
        {
            mCurrentCamControl->setCamera(nullptr);
            mCurrentCamControl = mOrbitCamControl;
            mOrbitCamControl->setCamera(getCamera());
            mOrbitCamControl->reset();
        }
    }

}
