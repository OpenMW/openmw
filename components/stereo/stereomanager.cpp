#include "stereomanager.hpp"
#include "frustum.hpp"
#include "multiview.hpp"

#include <osg/DisplaySettings>
#include <osg/Texture2D>
#include <osg/Texture2DArray>
#include <osg/Texture2DMultisample>
#include <osg/io_utils>

#include <osgUtil/CullVisitor>
#include <osgUtil/RenderStage>

#include <osgViewer/Renderer>
#include <osgViewer/Viewer>

#include <map>
#include <string>

#include <components/misc/constants.hpp>

#include <components/sceneutil/color.hpp>
#include <components/sceneutil/depth.hpp>
#include <components/sceneutil/statesetupdater.hpp>

#include <components/settings/values.hpp>

namespace Stereo
{
    // Update stereo view/projection during update
    class StereoUpdateCallback final : public osg::Callback
    {
    public:
        StereoUpdateCallback(Manager* stereoView)
            : stereoView(stereoView)
        {
        }

        bool run(osg::Object* object, osg::Object* data) override
        {
            auto b = traverse(object, data);
            stereoView->update();
            return b;
        }

        Manager* stereoView;
    };

    // Update states during cull
    class BruteForceStereoStatesetUpdateCallback final : public SceneUtil::StateSetUpdater
    {
    public:
        BruteForceStereoStatesetUpdateCallback(Manager* manager)
            : mManager(manager)
        {
        }

    protected:
        virtual void setDefaults(osg::StateSet* stateset) override
        {
            stateset->addUniform(new osg::Uniform("projectionMatrix", osg::Matrixf{}));
        }

        virtual void apply(osg::StateSet* stateset, osg::NodeVisitor* /*nv*/) override {}

        void applyLeft(osg::StateSet* stateset, osgUtil::CullVisitor* nv) override
        {
            auto* uProjectionMatrix = stateset->getUniform("projectionMatrix");
            if (uProjectionMatrix)
                uProjectionMatrix->set(mManager->computeEyeProjection(0, SceneUtil::AutoDepth::isReversed()));
        }

        void applyRight(osg::StateSet* stateset, osgUtil::CullVisitor* nv) override
        {
            auto* uProjectionMatrix = stateset->getUniform("projectionMatrix");
            if (uProjectionMatrix)
                uProjectionMatrix->set(mManager->computeEyeProjection(1, SceneUtil::AutoDepth::isReversed()));
        }

    private:
        Manager* mManager;
    };

    // Update states during cull
    class MultiviewStereoStatesetUpdateCallback : public SceneUtil::StateSetUpdater
    {
    public:
        MultiviewStereoStatesetUpdateCallback(Manager* manager)
            : mManager(manager)
        {
        }

    protected:
        virtual void setDefaults(osg::StateSet* stateset)
        {
            stateset->addUniform(new osg::Uniform(osg::Uniform::FLOAT_MAT4, "invProjectionMatrixMultiView", 2));
        }

        virtual void apply(osg::StateSet* stateset, osg::NodeVisitor* /*nv*/)
        {
            mManager->updateMultiviewStateset(stateset);
        }

    private:
        Manager* mManager;
    };

    static bool sStereoEnabled = false;

    static Manager* sInstance = nullptr;

    Manager& Manager::instance()
    {
        return *sInstance;
    }

    Manager::Manager(osgViewer::Viewer* viewer, bool enableStereo)
        : mViewer(viewer)
        , mMainCamera(mViewer->getCamera())
        , mUpdateCallback(new StereoUpdateCallback(this))
        , mMasterProjectionMatrix(osg::Matrixd::identity())
        , mEyeResolutionOverriden(false)
        , mEyeResolutionOverride(0, 0)
        , mFrustumManager(nullptr)
        , mUpdateViewCallback(nullptr)
    {
        if (sInstance)
            throw std::logic_error("Double instance of Stereo::Manager");
        sInstance = this;
        sStereoEnabled = enableStereo;
    }

    Manager::~Manager() {}

    void Manager::initializeStereo(osg::GraphicsContext* gc, bool enableMultiview)
    {
        auto ci = gc->getState()->getContextID();
        configureExtensions(ci, enableMultiview);

        mMainCamera->addUpdateCallback(mUpdateCallback);
        mFrustumManager = std::make_unique<StereoFrustumManager>(mViewer->getCamera());

        if (getMultiview())
            setupOVRMultiView2Technique();
        else
            setupBruteForceTechnique();

        updateStereoFramebuffer();
    }

    void shaderStereoDefines(Shader::ShaderManager::DefineMap& defines)
    {
        if (getMultiview())
        {
            defines["useOVR_multiview"] = "1";
            defines["numViews"] = "2";
        }
        else
        {
            defines["useOVR_multiview"] = "0";
            defines["numViews"] = "1";
        }
    }

    void Manager::overrideEyeResolution(const osg::Vec2i& eyeResolution)
    {
        mEyeResolutionOverride = eyeResolution;
        mEyeResolutionOverriden = true;

        // if (mMultiviewFramebuffer)
        //     updateStereoFramebuffer();
    }

    void Manager::screenResolutionChanged()
    {
        updateStereoFramebuffer();
    }

    osg::Vec2i Manager::eyeResolution()
    {
        if (mEyeResolutionOverriden)
            return mEyeResolutionOverride;
        auto width = mMainCamera->getViewport()->width() / 2;
        auto height = mMainCamera->getViewport()->height();

        return osg::Vec2i(width, height);
    }

    void Manager::disableStereoForNode(osg::Node* node)
    {
        // Re-apply the main camera's full viewport to return to full screen rendering.
        node->getOrCreateStateSet()->setAttribute(mMainCamera->getViewport());
    }

    void Manager::setShadowTechnique(SceneUtil::MWShadowTechnique* shadowTechnique)
    {
        if (mFrustumManager)
            mFrustumManager->setShadowTechnique(shadowTechnique);
    }

    void Manager::setupBruteForceTechnique()
    {
        auto* ds = osg::DisplaySettings::instance().get();
        ds->setStereo(true);
        ds->setStereoMode(osg::DisplaySettings::StereoMode::HORIZONTAL_SPLIT);
        ds->setUseSceneViewForStereoHint(true);

        mMainCamera->addCullCallback(new BruteForceStereoStatesetUpdateCallback(this));

        struct ComputeStereoMatricesCallback : public osgUtil::SceneView::ComputeStereoMatricesCallback
        {
            ComputeStereoMatricesCallback(Manager* sv)
                : mManager(sv)
            {
            }

            osg::Matrixd computeLeftEyeProjection(const osg::Matrixd& projection) const override
            {
                (void)projection;
                return mManager->computeEyeProjection(0, false);
            }

            osg::Matrixd computeLeftEyeView(const osg::Matrixd& view) const override
            {
                return view * mManager->computeEyeViewOffset(0);
            }

            osg::Matrixd computeRightEyeProjection(const osg::Matrixd& projection) const override
            {
                (void)projection;
                return mManager->computeEyeProjection(1, false);
            }

            osg::Matrixd computeRightEyeView(const osg::Matrixd& view) const override
            {
                return view * mManager->computeEyeViewOffset(1);
            }

            Manager* mManager;
        };

        auto* renderer = static_cast<osgViewer::Renderer*>(mMainCamera->getRenderer());
        for (auto* sceneView : { renderer->getSceneView(0), renderer->getSceneView(1) })
        {
            sceneView->setComputeStereoMatricesCallback(new ComputeStereoMatricesCallback(this));

            auto* cvMain = sceneView->getCullVisitor();
            auto* cvLeft = sceneView->getCullVisitorLeft();
            auto* cvRight = sceneView->getCullVisitorRight();
            if (!cvMain)
                sceneView->setCullVisitor(cvMain = new osgUtil::CullVisitor());
            if (!cvLeft)
                sceneView->setCullVisitor(cvLeft = cvMain->clone());
            if (!cvRight)
                sceneView->setCullVisitor(cvRight = cvMain->clone());

            // Osg by default gives cullVisitorLeft and cullVisitor the same identifier.
            // So we make our own to avoid confusion
            cvMain->setIdentifier(mIdentifierMain);
            cvLeft->setIdentifier(mIdentifierLeft);
            cvRight->setIdentifier(mIdentifierRight);
        }
    }

    void Manager::setupOVRMultiView2Technique()
    {
        auto* ds = osg::DisplaySettings::instance().get();
        ds->setStereo(false);

        mMainCamera->addCullCallback(new MultiviewStereoStatesetUpdateCallback(this));
    }

    void Manager::updateStereoFramebuffer()
    {
        // VR-TODO: in VR, still need to have this framebuffer attached before the postprocessor is created
        // auto samples = /*do not use Settings here*/;
        // auto eyeRes = eyeResolution();

        // if (mMultiviewFramebuffer)
        //     mMultiviewFramebuffer->detachFrom(mMainCamera);
        // mMultiviewFramebuffer = std::make_shared<MultiviewFramebuffer>(static_cast<int>(eyeRes.x()),
        // static_cast<int>(eyeRes.y()), samples);
        // mMultiviewFramebuffer->attachColorComponent(SceneUtil::Color::colorSourceFormat(),
        // SceneUtil::Color::colorSourceType(), SceneUtil::Color::colorInternalFormat());
        // mMultiviewFramebuffer->attachDepthComponent(SceneUtil::AutoDepth::depthSourceFormat(),
        // SceneUtil::AutoDepth::depthSourceType(), SceneUtil::AutoDepth::depthInternalFormat());
        // mMultiviewFramebuffer->attachTo(mMainCamera);
    }

    void Manager::update()
    {
        const double near_ = Settings::camera().mNearClip;
        const double far_ = Settings::camera().mViewingDistance;

        if (mUpdateViewCallback)
        {
            mUpdateViewCallback->updateView(mView[0], mView[1]);
            mViewOffsetMatrix[0] = mView[0].viewMatrix(true);
            mViewOffsetMatrix[1] = mView[1].viewMatrix(true);
            mProjectionMatrix[0] = mView[0].perspectiveMatrix(near_, far_, false);
            mProjectionMatrix[1] = mView[1].perspectiveMatrix(near_, far_, false);
            if (SceneUtil::AutoDepth::isReversed())
            {
                mProjectionMatrixReverseZ[0] = mView[0].perspectiveMatrix(near_, far_, true);
                mProjectionMatrixReverseZ[1] = mView[1].perspectiveMatrix(near_, far_, true);
            }

            View masterView;
            masterView.fov.angleDown = std::min(mView[0].fov.angleDown, mView[1].fov.angleDown);
            masterView.fov.angleUp = std::max(mView[0].fov.angleUp, mView[1].fov.angleUp);
            masterView.fov.angleLeft = std::min(mView[0].fov.angleLeft, mView[1].fov.angleLeft);
            masterView.fov.angleRight = std::max(mView[0].fov.angleRight, mView[1].fov.angleRight);
            auto projectionMatrix = masterView.perspectiveMatrix(near_, far_, false);
            mMainCamera->setProjectionMatrix(projectionMatrix);
        }
        else
        {
            auto* ds = osg::DisplaySettings::instance().get();
            auto viewMatrix = mMainCamera->getViewMatrix();
            auto projectionMatrix = mMainCamera->getProjectionMatrix();
            auto s = ds->getEyeSeparation() * Constants::UnitsPerMeter;
            mViewOffsetMatrix[0]
                = osg::Matrixd(1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, s, 0.0, 0.0, 1.0);
            mViewOffsetMatrix[1]
                = osg::Matrixd(1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, -s, 0.0, 0.0, 1.0);
            mProjectionMatrix[0] = ds->computeLeftEyeProjectionImplementation(projectionMatrix);
            mProjectionMatrix[1] = ds->computeRightEyeProjectionImplementation(projectionMatrix);
            if (SceneUtil::AutoDepth::isReversed())
            {
                mProjectionMatrixReverseZ[0] = ds->computeLeftEyeProjectionImplementation(mMasterProjectionMatrix);
                mProjectionMatrixReverseZ[1] = ds->computeRightEyeProjectionImplementation(mMasterProjectionMatrix);
            }
        }

        mFrustumManager->update({ mProjectionMatrix[0], mProjectionMatrix[1] });
    }

    void Manager::updateMultiviewStateset(osg::StateSet* stateset)
    {
        std::array<osg::Matrix, 2> projectionMatrices;

        for (int view : { 0, 1 })
            projectionMatrices[view]
                = computeEyeViewOffset(view) * computeEyeProjection(view, SceneUtil::AutoDepth::isReversed());

        Stereo::setMultiviewMatrices(stateset, projectionMatrices, true);
    }

    void Manager::setUpdateViewCallback(std::shared_ptr<UpdateViewCallback> cb)
    {
        mUpdateViewCallback = std::move(cb);
    }

    void Manager::setCullCallback(osg::ref_ptr<osg::NodeCallback> cb)
    {
        mMainCamera->setCullCallback(cb);
    }

    osg::Matrixd Manager::computeEyeProjection(int view, bool reverseZ) const
    {
        return reverseZ ? mProjectionMatrixReverseZ[view] : mProjectionMatrix[view];
    }

    osg::Matrixd Manager::computeEyeViewOffset(int view) const
    {
        return mViewOffsetMatrix[view];
    }

    Eye Manager::getEye(const osgUtil::CullVisitor* cv) const
    {
        if (cv->getIdentifier() == mIdentifierMain)
            return Eye::Center;
        if (cv->getIdentifier() == mIdentifierLeft)
            return Eye::Left;
        if (cv->getIdentifier() == mIdentifierRight)
            return Eye::Right;
        return Eye::Center;
    }

    bool getStereo()
    {
        return sStereoEnabled;
    }

    Manager::CustomViewCallback::CustomViewCallback(View& left, View& right)
        : mLeft(left)
        , mRight(right)
    {
    }

    void Manager::CustomViewCallback::updateView(View& left, View& right)
    {
        left = mLeft;
        right = mRight;
    }

    InitializeStereoOperation::InitializeStereoOperation()
        : GraphicsOperation("InitializeStereoOperation", false)
    {
        // Ideally, this would have belonged to the operator(). But the vertex buffer
        // hint has to be set before realize is called on the osg viewer, and so has to
        // be done here instead.
        Stereo::setVertexBufferHint(Settings::Manager::getBool("multiview", "Stereo"));
    }

    void InitializeStereoOperation::operator()(osg::GraphicsContext* graphicsContext)
    {
        auto& sm = Stereo::Manager::instance();

        if (Settings::Manager::getBool("use custom view", "Stereo"))
        {
            Stereo::View left;
            Stereo::View right;

            left.pose.position.x() = Settings::Manager::getDouble("left eye offset x", "Stereo View");
            left.pose.position.y() = Settings::Manager::getDouble("left eye offset y", "Stereo View");
            left.pose.position.z() = Settings::Manager::getDouble("left eye offset z", "Stereo View");
            left.pose.orientation.x() = Settings::Manager::getDouble("left eye orientation x", "Stereo View");
            left.pose.orientation.y() = Settings::Manager::getDouble("left eye orientation y", "Stereo View");
            left.pose.orientation.z() = Settings::Manager::getDouble("left eye orientation z", "Stereo View");
            left.pose.orientation.w() = Settings::Manager::getDouble("left eye orientation w", "Stereo View");
            left.fov.angleLeft = Settings::Manager::getDouble("left eye fov left", "Stereo View");
            left.fov.angleRight = Settings::Manager::getDouble("left eye fov right", "Stereo View");
            left.fov.angleUp = Settings::Manager::getDouble("left eye fov up", "Stereo View");
            left.fov.angleDown = Settings::Manager::getDouble("left eye fov down", "Stereo View");

            right.pose.position.x() = Settings::Manager::getDouble("right eye offset x", "Stereo View");
            right.pose.position.y() = Settings::Manager::getDouble("right eye offset y", "Stereo View");
            right.pose.position.z() = Settings::Manager::getDouble("right eye offset z", "Stereo View");
            right.pose.orientation.x() = Settings::Manager::getDouble("right eye orientation x", "Stereo View");
            right.pose.orientation.y() = Settings::Manager::getDouble("right eye orientation y", "Stereo View");
            right.pose.orientation.z() = Settings::Manager::getDouble("right eye orientation z", "Stereo View");
            right.pose.orientation.w() = Settings::Manager::getDouble("right eye orientation w", "Stereo View");
            right.fov.angleLeft = Settings::Manager::getDouble("right eye fov left", "Stereo View");
            right.fov.angleRight = Settings::Manager::getDouble("right eye fov right", "Stereo View");
            right.fov.angleUp = Settings::Manager::getDouble("right eye fov up", "Stereo View");
            right.fov.angleDown = Settings::Manager::getDouble("right eye fov down", "Stereo View");

            auto customViewCallback = std::make_shared<Stereo::Manager::CustomViewCallback>(left, right);
            sm.setUpdateViewCallback(customViewCallback);
        }

        if (Settings::Manager::getBool("use custom eye resolution", "Stereo"))
        {
            osg::Vec2i eyeResolution = osg::Vec2i();
            eyeResolution.x() = Settings::Manager::getInt("eye resolution x", "Stereo View");
            eyeResolution.y() = Settings::Manager::getInt("eye resolution y", "Stereo View");
            sm.overrideEyeResolution(eyeResolution);
        }

        sm.initializeStereo(graphicsContext, Settings::Manager::getBool("multiview", "Stereo"));
    }
}
