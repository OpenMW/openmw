#include "precipitationocclusion.hpp"

#include <cassert>

#include <osgUtil/CullVisitor>

#include <components/misc/constants.hpp>
#include <components/resource/resourcesystem.hpp>
#include <components/resource/scenemanager.hpp>
#include <components/sceneutil/depth.hpp>
#include <components/sceneutil/positionattitudetransform.hpp>
#include <components/sceneutil/util.hpp>
#include <components/settings/values.hpp>
#include <components/shader/shadermanager.hpp>

#include "../mwbase/environment.hpp"

#include "vismask.hpp"

namespace
{
    class PrecipitationOcclusionUpdater : public SceneUtil::StateSetUpdater
    {
    public:
        PrecipitationOcclusionUpdater(osg::ref_ptr<osg::Texture2D> depthTexture)
            : mDepthTexture(std::move(depthTexture))
        {
        }

    private:
        void setDefaults(osg::StateSet* stateset) override
        {
            stateset->setTextureAttributeAndModes(3, mDepthTexture);
            stateset->addUniform(new osg::Uniform("orthoDepthMap", 3));
            stateset->addUniform(new osg::Uniform("depthSpaceMatrix", mDepthSpaceMatrix));
        }
        void apply(osg::StateSet* stateset, osg::NodeVisitor* nv) override
        {
            osg::Camera* camera = nv->asCullVisitor()->getCurrentCamera();
            stateset->getUniform("depthSpaceMatrix")->set(camera->getViewMatrix() * camera->getProjectionMatrix());
        }

        osg::Matrixf mDepthSpaceMatrix;
        osg::ref_ptr<osg::Texture2D> mDepthTexture;
    };

    class DepthCameraUpdater : public SceneUtil::StateSetUpdater
    {
    public:
        DepthCameraUpdater()
            : mDummyTexture(new osg::Texture2D)
        {
            mDummyTexture->setInternalFormat(GL_RGB);
            mDummyTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
            mDummyTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
            mDummyTexture->setTextureSize(1, 1);

            Shader::ShaderManager& shaderMgr
                = MWBase::Environment::get().getResourceSystem()->getSceneManager()->getShaderManager();
            mProgram = shaderMgr.getProgram("depthclipped");
        }

    private:
        void setDefaults(osg::StateSet* stateset) override
        {
            stateset->addUniform(new osg::Uniform("projectionMatrix", osg::Matrixf()));
            stateset->setAttributeAndModes(mProgram, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
            stateset->setTextureAttributeAndModes(0, mDummyTexture);
            stateset->setRenderBinDetails(
                osg::StateSet::OPAQUE_BIN, "RenderBin", osg::StateSet::OVERRIDE_PROTECTED_RENDERBIN_DETAILS);
        }
        void apply(osg::StateSet* stateset, osg::NodeVisitor* nv) override
        {
            osg::Camera* camera = nv->asCullVisitor()->getCurrentCamera();
            stateset->getUniform("projectionMatrix")->set(camera->getProjectionMatrix());
        }

        osg::Matrixf mProjectionMatrix;
        osg::ref_ptr<osg::Texture2D> mDummyTexture;
        osg::ref_ptr<osg::Program> mProgram;
    };
}

namespace MWRender
{
    PrecipitationOccluder::PrecipitationOccluder(
        osg::Group* skyNode, osg::Group* sceneNode, osg::Group* rootNode, osg::Camera* camera)
        : mSkyNode(skyNode)
        , mSceneNode(sceneNode)
        , mRootNode(rootNode)
        , mSceneCamera(camera)
    {
        constexpr int rttSize = 256;

        mDepthTexture = new osg::Texture2D;
        mDepthTexture->setTextureSize(rttSize, rttSize);
        mDepthTexture->setSourceFormat(GL_DEPTH_COMPONENT);
        mDepthTexture->setInternalFormat(GL_DEPTH_COMPONENT24);
        mDepthTexture->setSourceType(GL_UNSIGNED_INT);
        mDepthTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_BORDER);
        mDepthTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_BORDER);
        mDepthTexture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
        mDepthTexture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
        mDepthTexture->setBorderColor(
            SceneUtil::AutoDepth::isReversed() ? osg::Vec4(0, 0, 0, 0) : osg::Vec4(1, 1, 1, 1));

        mCamera = new osg::Camera;
        mCamera->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);
        mCamera->setRenderOrder(osg::Camera::PRE_RENDER);
        mCamera->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);
        mCamera->setReferenceFrame(osg::Camera::ABSOLUTE_RF_INHERIT_VIEWPOINT);
        mCamera->setNodeMask(Mask_RenderToTexture);
        mCamera->setCullMask(Mask_Scene | Mask_Object | Mask_Static);
        mCamera->setViewport(0, 0, rttSize, rttSize);
        mCamera->attach(osg::Camera::DEPTH_BUFFER, mDepthTexture);
        mCamera->addChild(mSceneNode);
        mCamera->setSmallFeatureCullingPixelSize(
            Settings::shaders().mWeatherParticleOcclusionSmallFeatureCullingPixelSize);

        SceneUtil::setCameraClearDepth(mCamera);
    }

    void PrecipitationOccluder::update()
    {
        if (!mRange.has_value())
            return;

        const osg::Vec3 pos = mSceneCamera->getInverseViewMatrix().getTrans();

        const float zmin = pos.z() - mRange->z() - Constants::CellSizeInUnits;
        const float zmax = pos.z() + mRange->z() + Constants::CellSizeInUnits;
        const float near = 0;
        const float far = zmax - zmin;

        const float left = -mRange->x() / 2;
        const float right = -left;
        const float top = mRange->y() / 2;
        const float bottom = -top;

        if (SceneUtil::AutoDepth::isReversed())
        {
            mCamera->setProjectionMatrix(
                SceneUtil::getReversedZProjectionMatrixAsOrtho(left, right, bottom, top, near, far));
        }
        else
        {
            mCamera->setProjectionMatrix(osg::Matrixf::ortho(left, right, bottom, top, near, far));
        }

        mCamera->setViewMatrixAsLookAt(
            osg::Vec3(pos.x(), pos.y(), zmax), osg::Vec3(pos.x(), pos.y(), zmin), osg::Vec3(0, 1, 0));
    }

    void PrecipitationOccluder::enable()
    {
        mSkyCullCallback = new PrecipitationOcclusionUpdater(mDepthTexture);
        mSkyNode->addCullCallback(mSkyCullCallback);
        mCamera->setCullCallback(new DepthCameraUpdater);

        mRootNode->removeChild(mCamera);
        mRootNode->addChild(mCamera);
    }

    void PrecipitationOccluder::disable()
    {
        mSkyNode->removeCullCallback(mSkyCullCallback);
        mCamera->setCullCallback(nullptr);
        mSkyCullCallback = nullptr;

        mRootNode->removeChild(mCamera);
        mRange = std::nullopt;
    }

    void PrecipitationOccluder::updateRange(const osg::Vec3f range)
    {
        assert(range.x() != 0);
        assert(range.y() != 0);
        assert(range.z() != 0);
        const osg::Vec3f margin = { -50, -50, 0 };
        mRange = range - margin;
    }
}
