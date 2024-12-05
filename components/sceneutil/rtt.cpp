#include "rtt.hpp"
#include "util.hpp"

#include <osg/Texture2D>
#include <osg/Texture2DArray>
#include <osgUtil/CullVisitor>

#include <components/sceneutil/color.hpp>
#include <components/sceneutil/depth.hpp>
#include <components/sceneutil/nodecallback.hpp>
#include <components/stereo/multiview.hpp>
#include <components/stereo/stereomanager.hpp>

namespace SceneUtil
{
    class CullCallback : public SceneUtil::NodeCallback<CullCallback, RTTNode*, osgUtil::CullVisitor*>
    {
    public:
        void operator()(RTTNode* node, osgUtil::CullVisitor* cv) { node->cull(cv); }
    };

    RTTNode::RTTNode(uint32_t textureWidth, uint32_t textureHeight, uint32_t samples, bool generateMipmaps,
        int renderOrderNum, StereoAwareness stereoAwareness, bool addMSAAIntermediateTarget)
        : mTextureWidth(textureWidth)
        , mTextureHeight(textureHeight)
        , mSamples(samples)
        , mGenerateMipmaps(generateMipmaps)
        , mColorBufferInternalFormat(Color::colorInternalFormat())
        , mDepthBufferInternalFormat(SceneUtil::AutoDepth::depthInternalFormat())
        , mRenderOrderNum(renderOrderNum)
        , mStereoAwareness(stereoAwareness)
        , mAddMSAAIntermediateTarget(addMSAAIntermediateTarget)
    {
        addCullCallback(new CullCallback);
        setCullingActive(false);
    }

    RTTNode::~RTTNode()
    {
        for (auto& vdd : mViewDependentDataMap)
        {
            auto* camera = vdd.second->mCamera.get();
            if (camera)
            {
                camera->removeChildren(0, camera->getNumChildren());
            }
        }
        mViewDependentDataMap.clear();
    }

    void RTTNode::cull(osgUtil::CullVisitor* cv)
    {
        auto frameNumber = cv->getFrameStamp()->getFrameNumber();
        auto* vdd = getViewDependentData(cv);
        if (frameNumber > vdd->mFrameNumber)
        {
            apply(vdd->mCamera);
            if (Stereo::getStereo())
            {
                auto& sm = Stereo::Manager::instance();
                if (sm.getEye(cv) == Stereo::Eye::Left)
                    applyLeft(vdd->mCamera);
                if (sm.getEye(cv) == Stereo::Eye::Right)
                    applyRight(vdd->mCamera);
            }
            vdd->mCamera->accept(*cv);
        }
        vdd->mFrameNumber = frameNumber;
    }

    void RTTNode::setColorBufferInternalFormat(GLint internalFormat)
    {
        mColorBufferInternalFormat = internalFormat;
    }

    void RTTNode::setDepthBufferInternalFormat(GLint internalFormat)
    {
        mDepthBufferInternalFormat = internalFormat;
    }

    bool RTTNode::shouldDoPerViewMapping()
    {
        if (mStereoAwareness != StereoAwareness::Aware)
            return false;
        if (!Stereo::getMultiview())
            return true;
        return false;
    }

    bool RTTNode::shouldDoTextureArray()
    {
        if (mStereoAwareness == StereoAwareness::Unaware)
            return false;
        if (Stereo::getMultiview())
            return true;
        return false;
    }

    bool RTTNode::shouldDoTextureView()
    {
        if (mStereoAwareness != StereoAwareness::Unaware_MultiViewShaders)
            return false;
        if (Stereo::getMultiview())
            return true;
        return false;
    }

    osg::Texture2DArray* RTTNode::createTextureArray(GLint internalFormat)
    {
        osg::Texture2DArray* textureArray = new osg::Texture2DArray;
        textureArray->setTextureSize(mTextureWidth, mTextureHeight, 2);
        textureArray->setInternalFormat(internalFormat);
        GLenum sourceFormat = 0;
        GLenum sourceType = 0;
        if (SceneUtil::isDepthFormat(internalFormat))
        {
            SceneUtil::getDepthFormatSourceFormatAndType(internalFormat, sourceFormat, sourceType);
        }
        else
        {
            SceneUtil::getColorFormatSourceFormatAndType(internalFormat, sourceFormat, sourceType);
        }
        textureArray->setSourceFormat(sourceFormat);
        textureArray->setSourceType(sourceType);
        textureArray->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
        textureArray->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
        textureArray->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
        textureArray->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
        textureArray->setWrap(osg::Texture::WRAP_R, osg::Texture::CLAMP_TO_EDGE);
        return textureArray;
    }

    osg::Texture2D* RTTNode::createTexture(GLint internalFormat)
    {
        osg::Texture2D* texture = new osg::Texture2D;
        texture->setTextureSize(mTextureWidth, mTextureHeight);
        texture->setInternalFormat(internalFormat);
        GLenum sourceFormat = 0;
        GLenum sourceType = 0;
        if (SceneUtil::isDepthFormat(internalFormat))
        {
            SceneUtil::getDepthFormatSourceFormatAndType(internalFormat, sourceFormat, sourceType);
        }
        else
        {
            SceneUtil::getColorFormatSourceFormatAndType(internalFormat, sourceFormat, sourceType);
        }
        texture->setSourceFormat(sourceFormat);
        texture->setSourceType(sourceType);
        texture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
        texture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
        texture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
        texture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
        texture->setWrap(osg::Texture::WRAP_R, osg::Texture::CLAMP_TO_EDGE);
        return texture;
    }

    osg::Texture* RTTNode::getColorTexture(osgUtil::CullVisitor* cv)
    {
        return getViewDependentData(cv)->mColorTexture;
    }

    osg::Texture* RTTNode::getDepthTexture(osgUtil::CullVisitor* cv)
    {
        return getViewDependentData(cv)->mDepthTexture;
    }

    osg::Camera* RTTNode::getCamera(osgUtil::CullVisitor* cv)
    {
        return getViewDependentData(cv)->mCamera;
    }

    RTTNode::ViewDependentData* RTTNode::getViewDependentData(osgUtil::CullVisitor* cv)
    {
        if (!shouldDoPerViewMapping())
            // Always setting it to null is an easy way to disable per-view mapping when mDoPerViewMapping is false.
            // This is safe since the visitor is never dereferenced.
            cv = nullptr;

        if (mViewDependentDataMap.count(cv) == 0)
        {
            auto camera = new osg::Camera();
            auto vdd = std::make_shared<ViewDependentData>();
            mViewDependentDataMap[cv] = vdd;
            mViewDependentDataMap[cv]->mCamera = camera;

            camera->setRenderOrder(osg::Camera::PRE_RENDER, mRenderOrderNum);
            camera->setClearMask(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
            camera->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);
            camera->setViewport(0, 0, mTextureWidth, mTextureHeight);
            SceneUtil::setCameraClearDepth(camera);

            setDefaults(camera);

            if (camera->getBufferAttachmentMap().count(osg::Camera::COLOR_BUFFER))
                vdd->mColorTexture = camera->getBufferAttachmentMap()[osg::Camera::COLOR_BUFFER]._texture;
            if (camera->getBufferAttachmentMap().count(osg::Camera::PACKED_DEPTH_STENCIL_BUFFER))
                vdd->mDepthTexture
                    = camera->getBufferAttachmentMap()[osg::Camera::PACKED_DEPTH_STENCIL_BUFFER]._texture;

            if (shouldDoTextureArray())
            {
                // Create any buffer attachments not added in setDefaults
                if (camera->getBufferAttachmentMap().count(osg::Camera::COLOR_BUFFER) == 0)
                {
                    vdd->mColorTexture = createTextureArray(mColorBufferInternalFormat);
                    camera->attach(osg::Camera::COLOR_BUFFER, vdd->mColorTexture, 0,
                        Stereo::osgFaceControlledByMultiviewShader(), mGenerateMipmaps, mSamples);
                    SceneUtil::attachAlphaToCoverageFriendlyFramebufferToCamera(camera, osg::Camera::COLOR_BUFFER,
                        vdd->mColorTexture, 0, Stereo::osgFaceControlledByMultiviewShader(), mGenerateMipmaps,
                        mAddMSAAIntermediateTarget);
                }

                if (camera->getBufferAttachmentMap().count(osg::Camera::PACKED_DEPTH_STENCIL_BUFFER) == 0)
                {
                    vdd->mDepthTexture = createTextureArray(mDepthBufferInternalFormat);
                    camera->attach(osg::Camera::PACKED_DEPTH_STENCIL_BUFFER, vdd->mDepthTexture, 0,
                        Stereo::osgFaceControlledByMultiviewShader(), false, mSamples);
                }

                if (shouldDoTextureView())
                {
                    // In this case, shaders being set to multiview forces us to render to a multiview framebuffer even
                    // though we don't need that. This forces us to make Texture2DArray. To make this possible to sample
                    // as a Texture2D, make a Texture2D view into the texture array.
                    vdd->mColorTexture = Stereo::createTextureView_Texture2DFromTexture2DArray(
                        static_cast<osg::Texture2DArray*>(vdd->mColorTexture.get()), 0);
                    vdd->mDepthTexture = Stereo::createTextureView_Texture2DFromTexture2DArray(
                        static_cast<osg::Texture2DArray*>(vdd->mDepthTexture.get()), 0);
                }
            }
            else
            {
                // Create any buffer attachments not added in setDefaults
                if (camera->getBufferAttachmentMap().count(osg::Camera::COLOR_BUFFER) == 0)
                {
                    vdd->mColorTexture = createTexture(mColorBufferInternalFormat);
                    camera->attach(osg::Camera::COLOR_BUFFER, vdd->mColorTexture, 0, 0, mGenerateMipmaps, mSamples);
                    SceneUtil::attachAlphaToCoverageFriendlyFramebufferToCamera(camera, osg::Camera::COLOR_BUFFER,
                        vdd->mColorTexture, 0, 0, mGenerateMipmaps, mAddMSAAIntermediateTarget);
                }

                if (camera->getBufferAttachmentMap().count(osg::Camera::PACKED_DEPTH_STENCIL_BUFFER) == 0)
                {
                    vdd->mDepthTexture = createTexture(mDepthBufferInternalFormat);
                    camera->attach(osg::Camera::PACKED_DEPTH_STENCIL_BUFFER, vdd->mDepthTexture, 0, 0, false, mSamples);
                }
            }

            // OSG appears not to properly initialize this metadata. So when multisampling is enabled, OSG will use
            // incorrect formats for the resolve buffers.
            if (mSamples > 1)
            {
                camera->getBufferAttachmentMap()[osg::Camera::COLOR_BUFFER]._internalFormat
                    = mColorBufferInternalFormat;
                camera->getBufferAttachmentMap()[osg::Camera::COLOR_BUFFER]._mipMapGeneration = mGenerateMipmaps;
                camera->getBufferAttachmentMap()[osg::Camera::PACKED_DEPTH_STENCIL_BUFFER]._internalFormat
                    = mDepthBufferInternalFormat;
                camera->getBufferAttachmentMap()[osg::Camera::PACKED_DEPTH_STENCIL_BUFFER]._mipMapGeneration
                    = mGenerateMipmaps;
            }
        }

        return mViewDependentDataMap[cv].get();
    }
}
