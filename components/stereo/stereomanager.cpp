#include "stereomanager.hpp"
#include "multiview.hpp"
#include "frustum.hpp"

#include <osg/io_utils>
#include <osg/Texture2D>
#include <osg/Texture2DMultisample>
#include <osg/Texture2DArray>
#include <osg/DisplaySettings>

#include <osgUtil/CullVisitor>
#include <osgUtil/RenderStage>

#include <osgViewer/Renderer>
#include <osgViewer/Viewer>

#include <iostream>
#include <map>
#include <string>

#include <components/debug/debuglog.hpp>

#include <components/sceneutil/statesetupdater.hpp>
#include <components/sceneutil/visitor.hpp>
#include <components/sceneutil/util.hpp>
#include <components/sceneutil/depth.hpp>
#include <components/sceneutil/color.hpp>

#include <components/settings/settings.hpp>

#include <components/misc/stringops.hpp>

namespace Stereo
{
    // Update stereo view/projection during update
    class StereoUpdateCallback final : public osg::Callback
    {
    public:
        StereoUpdateCallback(Manager* stereoView) : stereoView(stereoView) {}

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

        virtual void apply(osg::StateSet* stateset, osg::NodeVisitor* /*nv*/) override
        {
        }

        void applyLeft(osg::StateSet* stateset, osgUtil::CullVisitor* nv) override
        {
            osg::Matrix dummy;
            auto* uProjectionMatrix = stateset->getUniform("projectionMatrix");
            if (uProjectionMatrix)
                uProjectionMatrix->set(mManager->computeEyeProjection(0, SceneUtil::AutoDepth::isReversed()));
        }

        void applyRight(osg::StateSet* stateset, osgUtil::CullVisitor* nv) override
        {
            osg::Matrix dummy;
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
            stateset->addUniform(new osg::Uniform(osg::Uniform::FLOAT_MAT4, "viewMatrixMultiView", 2));
            stateset->addUniform(new osg::Uniform(osg::Uniform::FLOAT_MAT4, "projectionMatrixMultiView", 2));
        }

        virtual void apply(osg::StateSet* stateset, osg::NodeVisitor* /*nv*/)
        {
            mManager->updateMultiviewStateset(stateset);
        }

    private:
        Manager* mManager;
    };

    static Manager* sInstance = nullptr;

    Manager& Manager::instance()
    {
        return *sInstance;
    }

    struct CustomViewCallback : public Manager::UpdateViewCallback
    {
    public:
        CustomViewCallback();

        void updateView(View& left, View& right) override;

    private:
        View mLeft;
        View mRight;
    };

    Manager::Manager(osgViewer::Viewer* viewer)
        : mViewer(viewer)
        , mMainCamera(mViewer->getCamera())
        , mUpdateCallback(new StereoUpdateCallback(this))
        , mMasterProjectionMatrix(osg::Matrix::identity())
        , mEyeResolutionOverriden(false)
        , mEyeResolutionOverride(0,0)
        , mFrustumManager(nullptr)
        , mUpdateViewCallback(nullptr)
    {
        if (sInstance)
            throw std::logic_error("Double instance of Stereo::Manager");
        sInstance = this;

        if (Settings::Manager::getBool("use custom view", "Stereo"))
            mUpdateViewCallback = std::make_shared<CustomViewCallback>();

        if (Settings::Manager::getBool("use custom eye resolution", "Stereo"))
        {
            osg::Vec2i eyeResolution = osg::Vec2i();
            eyeResolution.x() = Settings::Manager::getInt("eye resolution x", "Stereo View");
            eyeResolution.y() = Settings::Manager::getInt("eye resolution y", "Stereo View");
            overrideEyeResolution(eyeResolution);
        }
    }

    Manager::~Manager()
    {
    }

    void Manager::initializeStereo(osg::GraphicsContext* gc)
    {
        mMainCamera->addUpdateCallback(mUpdateCallback);
        mFrustumManager = std::make_unique<StereoFrustumManager>(mViewer->getCamera());

        auto ci = gc->getState()->getContextID();
        configureExtensions(ci);

        if(getMultiview())
            setupOVRMultiView2Technique();
        else
            setupBruteForceTechnique();

        updateStereoFramebuffer();

    }

    void Manager::shaderStereoDefines(Shader::ShaderManager::DefineMap& defines) const
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

        if (mMultiviewFramebuffer)
            updateStereoFramebuffer();
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
                (void)view;
                return mManager->computeEyeView(0);
            }

            osg::Matrixd computeRightEyeProjection(const osg::Matrixd& projection) const override
            {
                (void)projection;
                return mManager->computeEyeProjection(1, false);
            }

            osg::Matrixd computeRightEyeView(const osg::Matrixd& view) const override
            {
                (void)view;
                return mManager->computeEyeView(1);
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
        auto samples = Settings::Manager::getInt("antialiasing", "Video");
        auto eyeRes = eyeResolution();

        if (mMultiviewFramebuffer)
            mMultiviewFramebuffer->detachFrom(mMainCamera);
        mMultiviewFramebuffer = std::make_shared<MultiviewFramebuffer>(static_cast<int>(eyeRes.x()), static_cast<int>(eyeRes.y()), samples);
        mMultiviewFramebuffer->attachColorComponent(SceneUtil::Color::colorSourceFormat(), SceneUtil::Color::colorSourceType(), SceneUtil::Color::colorInternalFormat());
        mMultiviewFramebuffer->attachDepthComponent(SceneUtil::AutoDepth::depthSourceFormat(), SceneUtil::AutoDepth::depthSourceType(), SceneUtil::AutoDepth::depthInternalFormat());
        mMultiviewFramebuffer->attachTo(mMainCamera);
    }

    void Manager::update()
    {
        double near_ = 1.f;
        double far_ = 10000.f;

        near_ = Settings::Manager::getFloat("near clip", "Camera");
        far_ = Settings::Manager::getFloat("viewing distance", "Camera");
        auto projectionMatrix = mMainCamera->getProjectionMatrix();

        if (mUpdateViewCallback)
        {
            mUpdateViewCallback->updateView(mView[0], mView[1]);
            auto viewMatrix = mMainCamera->getViewMatrix();
            mViewOffsetMatrix[0] = mView[0].viewMatrix(true);
            mViewOffsetMatrix[1] = mView[1].viewMatrix(true);
            mViewMatrix[0] = viewMatrix * mViewOffsetMatrix[0];
            mViewMatrix[1] = viewMatrix * mViewOffsetMatrix[1];
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
            projectionMatrix = masterView.perspectiveMatrix(near_, far_, false);
            mMainCamera->setProjectionMatrix(projectionMatrix);
        }
        else
        {
            auto* ds = osg::DisplaySettings::instance().get();
            auto viewMatrix = mMainCamera->getViewMatrix();
            mViewMatrix[0] = ds->computeLeftEyeViewImplementation(viewMatrix);
            mViewMatrix[1] = ds->computeRightEyeViewImplementation(viewMatrix);
            mViewOffsetMatrix[0] = osg::Matrix::inverse(viewMatrix) * mViewMatrix[0];
            mViewOffsetMatrix[1] = osg::Matrix::inverse(viewMatrix) * mViewMatrix[1];
            mProjectionMatrix[0] = ds->computeLeftEyeProjectionImplementation(projectionMatrix);
            mProjectionMatrix[1] = ds->computeRightEyeProjectionImplementation(projectionMatrix);
            if (SceneUtil::AutoDepth::isReversed())
            {
                mProjectionMatrixReverseZ[0] = ds->computeLeftEyeProjectionImplementation(mMasterProjectionMatrix);
                mProjectionMatrixReverseZ[1] = ds->computeRightEyeProjectionImplementation(mMasterProjectionMatrix);
            }
        }

        mFrustumManager->update(
            { 
                mViewOffsetMatrix[0] * mProjectionMatrix[0], 
                mViewOffsetMatrix[1] * mProjectionMatrix[1] 
            });
    }

    void Manager::updateMultiviewStateset(osg::StateSet* stateset)
    {
        // Update stereo uniforms
        auto * viewMatrixMultiViewUniform = stateset->getUniform("viewMatrixMultiView");
        auto * projectionMatrixMultiViewUniform = stateset->getUniform("projectionMatrixMultiView");

        for (int view : {0, 1})
        {
            viewMatrixMultiViewUniform->setElement(view, mViewOffsetMatrix[view]);
            projectionMatrixMultiViewUniform->setElement(view, computeEyeProjection(view, SceneUtil::AutoDepth::isReversed()));
        }
    }

    void Manager::setUpdateViewCallback(std::shared_ptr<UpdateViewCallback> cb)
    {
        mUpdateViewCallback = cb;
    }

    void Manager::setCullCallback(osg::ref_ptr<osg::NodeCallback> cb)
    {
        mMainCamera->setCullCallback(cb);
    }

    osg::Matrixd Manager::computeEyeProjection(int view, bool reverseZ) const
    {
        return reverseZ ? mProjectionMatrixReverseZ[view] : mProjectionMatrix[view];
    }

    osg::Matrixd Manager::computeEyeView(int view) const
    {
        return mViewMatrix[view];
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
        static bool stereo = Settings::Manager::getBool("stereo enabled", "Stereo") || osg::DisplaySettings::instance().get()->getStereo();
        return stereo;
    }

    CustomViewCallback::CustomViewCallback()
    {
        mLeft.pose.position.x() = Settings::Manager::getDouble("left eye offset x", "Stereo View");
        mLeft.pose.position.y() = Settings::Manager::getDouble("left eye offset y", "Stereo View");
        mLeft.pose.position.z() = Settings::Manager::getDouble("left eye offset z", "Stereo View");
        mLeft.pose.orientation.x() = Settings::Manager::getDouble("left eye orientation x", "Stereo View");
        mLeft.pose.orientation.y() = Settings::Manager::getDouble("left eye orientation y", "Stereo View");
        mLeft.pose.orientation.z() = Settings::Manager::getDouble("left eye orientation z", "Stereo View");
        mLeft.pose.orientation.w() = Settings::Manager::getDouble("left eye orientation w", "Stereo View");
        mLeft.fov.angleLeft = Settings::Manager::getDouble("left eye fov left", "Stereo View");
        mLeft.fov.angleRight = Settings::Manager::getDouble("left eye fov right", "Stereo View");
        mLeft.fov.angleUp = Settings::Manager::getDouble("left eye fov up", "Stereo View");
        mLeft.fov.angleDown = Settings::Manager::getDouble("left eye fov down", "Stereo View");

        mRight.pose.position.x() = Settings::Manager::getDouble("right eye offset x", "Stereo View");
        mRight.pose.position.y() = Settings::Manager::getDouble("right eye offset y", "Stereo View");
        mRight.pose.position.z() = Settings::Manager::getDouble("right eye offset z", "Stereo View");
        mRight.pose.orientation.x() = Settings::Manager::getDouble("right eye orientation x", "Stereo View");
        mRight.pose.orientation.y() = Settings::Manager::getDouble("right eye orientation y", "Stereo View");
        mRight.pose.orientation.z() = Settings::Manager::getDouble("right eye orientation z", "Stereo View");
        mRight.pose.orientation.w() = Settings::Manager::getDouble("right eye orientation w", "Stereo View");
        mRight.fov.angleLeft = Settings::Manager::getDouble("right eye fov left", "Stereo View");
        mRight.fov.angleRight = Settings::Manager::getDouble("right eye fov right", "Stereo View");
        mRight.fov.angleUp = Settings::Manager::getDouble("right eye fov up", "Stereo View");
        mRight.fov.angleDown = Settings::Manager::getDouble("right eye fov down", "Stereo View");
    }

    void CustomViewCallback::updateView(View& left, View& right)
    {
        left = mLeft;
        right = mRight;
    }
}
