#include "rtt.hpp"
#include "util.hpp"

#include <osg/Node>
#include <osg/NodeVisitor>
#include <osg/Texture2D>
#include <osgUtil/CullVisitor>

#include <components/settings/settings.hpp>

namespace SceneUtil
{
    // RTTNode's cull callback
    class CullCallback : public osg::NodeCallback
    {
    public:
        CullCallback(RTTNode* group)
            : mGroup(group) {}

        void operator()(osg::Node* node, osg::NodeVisitor* nv) override
        {
            osgUtil::CullVisitor* cv = static_cast<osgUtil::CullVisitor*>(nv);
            mGroup->cull(cv);
        }
        RTTNode* mGroup;
    };

    RTTNode::RTTNode(uint32_t textureWidth, uint32_t textureHeight, int renderOrderNum, bool doPerViewMapping)
        : mTextureWidth(textureWidth)
        , mTextureHeight(textureHeight)
        , mRenderOrderNum(renderOrderNum)
        , mDoPerViewMapping(doPerViewMapping)
    {
        addCullCallback(new CullCallback(this));
        setCullingActive(false);
    }

    RTTNode::~RTTNode()
    {
    }

    void RTTNode::cull(osgUtil::CullVisitor* cv)
    {
        auto* vdd = getViewDependentData(cv);
        apply(vdd->mCamera);
        vdd->mCamera->accept(*cv);
    }

    osg::Texture* RTTNode::getColorTexture(osgUtil::CullVisitor* cv)
    {
        return getViewDependentData(cv)->mCamera->getBufferAttachmentMap()[osg::Camera::COLOR_BUFFER]._texture;
    }

    osg::Texture* RTTNode::getDepthTexture(osgUtil::CullVisitor* cv)
    {
        return getViewDependentData(cv)->mCamera->getBufferAttachmentMap()[osg::Camera::DEPTH_BUFFER]._texture;
    }

    RTTNode::ViewDependentData* RTTNode::getViewDependentData(osgUtil::CullVisitor* cv)
    {
        if (!mDoPerViewMapping)
            // Always setting it to null is an easy way to disable per-view mapping when mDoPerViewMapping is false.
            // This is safe since the visitor is never dereferenced.
            cv = nullptr;

        if (mViewDependentDataMap.count(cv) == 0)
        {
            auto camera = new osg::Camera();
            mViewDependentDataMap[cv].reset(new ViewDependentData);
            mViewDependentDataMap[cv]->mCamera = camera;

            camera->setRenderOrder(osg::Camera::PRE_RENDER, mRenderOrderNum);
            camera->setClearMask(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
            camera->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);
            camera->setViewport(0, 0, mTextureWidth, mTextureHeight);

            setDefaults(mViewDependentDataMap[cv]->mCamera.get());

            // Create any buffer attachments not added in setDefaults
            if (camera->getBufferAttachmentMap().count(osg::Camera::COLOR_BUFFER) == 0)
            {
                auto colorBuffer = new osg::Texture2D;
                colorBuffer->setTextureSize(mTextureWidth, mTextureHeight);
                colorBuffer->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
                colorBuffer->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
                colorBuffer->setInternalFormat(GL_RGB);
                colorBuffer->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
                colorBuffer->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
                camera->attach(osg::Camera::COLOR_BUFFER, colorBuffer);
                SceneUtil::attachAlphaToCoverageFriendlyFramebufferToCamera(camera, osg::Camera::COLOR_BUFFER, colorBuffer);
            }

            if (camera->getBufferAttachmentMap().count(osg::Camera::DEPTH_BUFFER) == 0)
            {
                auto depthBuffer = new osg::Texture2D;
                depthBuffer->setTextureSize(mTextureWidth, mTextureHeight);
                depthBuffer->setSourceFormat(GL_DEPTH_COMPONENT);
                depthBuffer->setInternalFormat(GL_DEPTH_COMPONENT24);
                depthBuffer->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
                depthBuffer->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
                depthBuffer->setSourceType(GL_UNSIGNED_INT);
                depthBuffer->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
                depthBuffer->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
                camera->attach(osg::Camera::DEPTH_BUFFER, depthBuffer);
            }
        }

        return mViewDependentDataMap[cv].get();
    }
}
