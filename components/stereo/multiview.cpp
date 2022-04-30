#include "multiview.hpp"

#include <osg/FrameBufferObject>
#include <osg/GLExtensions>
#include <osg/Texture2D>
#include <osg/Texture2DMultisample>
#include <osg/Texture2DArray>
#include <osgUtil/RenderStage>
#include <osgUtil/CullVisitor>

#include <components/sceneutil/nodecallback.hpp>
#include <components/settings/settings.hpp>
#include <components/debug/debuglog.hpp>
#include <components/stereo/stereomanager.hpp>

#include <algorithm>

namespace Stereo
{
    namespace
    {
        bool getMultiviewSupportedImpl(unsigned int contextID)
        {
#ifdef OSG_HAS_MULTIVIEW
            if (!osg::isGLExtensionSupported(contextID, "GL_OVR_multiview"))
            {
                Log(Debug::Verbose) << "Disabling Multiview (opengl extension \"GL_OVR_multiview\" not supported)";
                return false;
            }

            if (!osg::isGLExtensionSupported(contextID, "GL_OVR_multiview2"))
            {
                Log(Debug::Verbose) << "Disabling Multiview (opengl extension \"GL_OVR_multiview2\" not supported)";
                return false;
            }
            return true;
#else
            Log(Debug::Verbose) << "Disabling Multiview (OSG does not support multiview)";
            return false;
#endif
        }

        bool getMultiviewSupported(unsigned int contextID)
        {
            static bool supported = getMultiviewSupportedImpl(contextID);
            return supported;
        }

        bool getTextureViewSupportedImpl(unsigned int contextID)
        {
            if (!osg::isGLExtensionOrVersionSupported(contextID, "ARB_texture_view", 4.3))
            {
                Log(Debug::Verbose) << "Disabling texture views (opengl extension \"ARB_texture_view\" not supported)";
                return false;
            }
            return true;
        }

        bool getTextureViewSupported(unsigned int contextID)
        {
            static bool supported = getTextureViewSupportedImpl(contextID);
            return supported;
        }

        bool getMultiviewImpl(unsigned int contextID)
        {
            if (!Stereo::getStereo())
            {
                Log(Debug::Verbose) << "Disabling Multiview (disabled by config)";
                return false;
            }

            if (!Settings::Manager::getBool("multiview", "Stereo"))
            {
                Log(Debug::Verbose) << "Disabling Multiview (disabled by config)";
                return false;
            }

            if (!getMultiviewSupported(contextID))
            {
                return false;
            }

            if (!getTextureViewSupported(contextID))
            {
                Log(Debug::Verbose) << "Disabling Multiview (texture views not supported)";
                return false;
            }

            Log(Debug::Verbose) << "Enabling Multiview";
            return true;
        }

        bool getMultiview(unsigned int contextID)
        {
            static bool multiView = getMultiviewImpl(contextID);
            return multiView;
        }
    }

    bool getTextureViewSupported()
    {
        return getTextureViewSupported(0);
    }

    bool getMultiview()
    {
        return getMultiview(0);
    }

    void configureExtensions(unsigned int contextID)
    {
        getTextureViewSupported(contextID);
        getMultiviewSupported(contextID);
        getMultiview(contextID);
    }

    void setVertexBufferHint()
    {
        if (getStereo() && Settings::Manager::getBool("multiview", "Stereo"))
        {
            auto* ds = osg::DisplaySettings::instance().get();
            if (!Settings::Manager::getBool("allow display lists for multiview", "Stereo")
                && ds->getVertexBufferHint() == osg::DisplaySettings::VertexBufferHint::NO_PREFERENCE)
            {
                // Note that this only works if this code is executed before realize() is called on the viewer.
                // The hint is read by the state object only once, before the user realize operations are run. 
                // Therefore we have to set this hint without access to a graphics context to let us determine 
                // if multiview will actually be supported or not. So if the user has requested multiview, we
                // will just have to set it regardless.
                ds->setVertexBufferHint(osg::DisplaySettings::VertexBufferHint::VERTEX_BUFFER_OBJECT);
                Log(Debug::Verbose) << "Disabling display lists";
            }
        }
    }

    class Texture2DViewSubloadCallback : public osg::Texture2D::SubloadCallback
    {
    public:
        Texture2DViewSubloadCallback(osg::Texture2DArray* textureArray, int layer);

        void load(const osg::Texture2D& texture, osg::State& state) const override;
        void subload(const osg::Texture2D& texture, osg::State& state) const override;

    private:
        osg::ref_ptr<osg::Texture2DArray> mTextureArray;
        int mLayer;
    };

    Texture2DViewSubloadCallback::Texture2DViewSubloadCallback(osg::Texture2DArray* textureArray, int layer)
        : mTextureArray(textureArray)
        , mLayer(layer)
    {
    }

    void Texture2DViewSubloadCallback::load(const osg::Texture2D& texture, osg::State& state) const
    {
        state.checkGLErrors("before Texture2DViewSubloadCallback::load()");

        auto contextId = state.getContextID();
        auto* gl = osg::GLExtensions::Get(contextId, false);
        mTextureArray->apply(state);

        auto sourceTextureObject = mTextureArray->getTextureObject(contextId);
        if (!sourceTextureObject)
        {
            Log(Debug::Error) << "Texture2DViewSubloadCallback: Texture2DArray did not have a texture object";
            return;
        }

        auto targetTextureObject = texture.getTextureObject(contextId);
        if (!sourceTextureObject)
        {
            Log(Debug::Error) << "Texture2DViewSubloadCallback: Texture2D did not have a texture object";
            return;
        }


        // OSG already bound this texture ID, giving it a target.
        // Delete it and make a new texture ID.
        glBindTexture(GL_TEXTURE_2D, 0);
        glDeleteTextures(1, &targetTextureObject->_id);
        glGenTextures(1, &targetTextureObject->_id);

        auto sourceId = sourceTextureObject->_id;
        auto targetId = targetTextureObject->_id;
        auto internalFormat = sourceTextureObject->_profile._internalFormat;
        auto levels = std::max(1, sourceTextureObject->_profile._numMipmapLevels);

        {
            ////// OSG BUG
            // Texture views require immutable storage. 
            // OSG should always give immutable storage to sized internal formats, but does not do so for depth formats.
            // Fortunately, we can just call glTexStorage3D here to make it immutable. This probably discards depth info for that frame, but whatever.
#ifndef GL_TEXTURE_IMMUTABLE_FORMAT
#define GL_TEXTURE_IMMUTABLE_FORMAT 0x912F 
#endif
            // Store any current binding and re-apply it after so i don't mess with state.
            GLint oldBinding = 0;
            glGetIntegerv(GL_TEXTURE_BINDING_2D_ARRAY, &oldBinding);

            // Bind the source texture and check if it's immutable.
            glBindTexture(GL_TEXTURE_2D_ARRAY, sourceId);
            GLint immutable = 0;
            glGetTexParameteriv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_IMMUTABLE_FORMAT, &immutable);
            if(!immutable)
            {
                // It wasn't immutable, so make it immutable.
                gl->glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, internalFormat, sourceTextureObject->_profile._width, sourceTextureObject->_profile._height, 2);
                state.checkGLErrors("after Texture2DViewSubloadCallback::load()::glTexStorage3D");
            }
            glBindTexture(GL_TEXTURE_2D_ARRAY, oldBinding);
        }

        gl->glTextureView(targetId, GL_TEXTURE_2D, sourceId, internalFormat, 0, levels, mLayer, 1);
        state.checkGLErrors("after Texture2DViewSubloadCallback::load()::glTextureView");
        glBindTexture(GL_TEXTURE_2D, targetId);
    }

    void Texture2DViewSubloadCallback::subload(const osg::Texture2D& texture, osg::State& state) const
    {
        // Nothing to do
    }

    osg::ref_ptr<osg::Texture2D> createTextureView_Texture2DFromTexture2DArray(osg::Texture2DArray* textureArray, int layer)
    {
        if (!getTextureViewSupported())
        {
            Log(Debug::Error) << "createTextureView_Texture2DFromTexture2DArray: Tried to use a texture view but glTextureView is not supported";
            return nullptr;
        }

        osg::ref_ptr<osg::Texture2D> texture2d = new osg::Texture2D;
        texture2d->setSubloadCallback(new Texture2DViewSubloadCallback(textureArray, layer));
        texture2d->setTextureSize(textureArray->getTextureWidth(), textureArray->getTextureHeight());
        texture2d->setBorderColor(textureArray->getBorderColor());
        texture2d->setBorderWidth(textureArray->getBorderWidth());
        texture2d->setLODBias(textureArray->getLODBias());
        texture2d->setFilter(osg::Texture::FilterParameter::MAG_FILTER, textureArray->getFilter(osg::Texture::FilterParameter::MAG_FILTER));
        texture2d->setFilter(osg::Texture::FilterParameter::MIN_FILTER, textureArray->getFilter(osg::Texture::FilterParameter::MIN_FILTER));
        texture2d->setInternalFormat(textureArray->getInternalFormat());
        texture2d->setNumMipmapLevels(textureArray->getNumMipmapLevels());
        return texture2d;
    }

    class UpdateRenderStagesCallback : public SceneUtil::NodeCallback<UpdateRenderStagesCallback, osg::Node*, osgUtil::CullVisitor*>
    {
    public:
        UpdateRenderStagesCallback(Stereo::MultiviewFramebuffer* multiviewFramebuffer)
            : mMultiviewFramebuffer(multiviewFramebuffer)
        {
            mViewport = new osg::Viewport(0, 0, multiviewFramebuffer->width(), multiviewFramebuffer->height());
            mViewportStateset = new osg::StateSet();
            mViewportStateset->setAttribute(mViewport.get());
        }

        void operator()(osg::Node* node, osgUtil::CullVisitor* cv)
        {
            osgUtil::RenderStage* renderStage = cv->getCurrentRenderStage();

            bool msaa = mMultiviewFramebuffer->samples() > 1;

            if (!Stereo::getMultiview())
            {
                auto eye = static_cast<int>(Stereo::Manager::instance().getEye(cv));

                if (msaa)
                {
                    renderStage->setFrameBufferObject(mMultiviewFramebuffer->layerMsaaFbo(eye));
                    renderStage->setMultisampleResolveFramebufferObject(mMultiviewFramebuffer->layerFbo(eye));
                }
                else
                {
                    renderStage->setFrameBufferObject(mMultiviewFramebuffer->layerFbo(eye));
                }
            }

            // OSG tries to do a horizontal split, but we want to render to separate framebuffers instead.
            renderStage->setViewport(mViewport);
            cv->pushStateSet(mViewportStateset.get());
            traverse(node, cv);
            cv->popStateSet();
        }

    private:
        Stereo::MultiviewFramebuffer* mMultiviewFramebuffer;
        osg::ref_ptr<osg::Viewport> mViewport;
        osg::ref_ptr<osg::StateSet> mViewportStateset;
    };

    MultiviewFramebuffer::MultiviewFramebuffer(int width, int height, int samples)
        : mWidth(width)
        , mHeight(height)
        , mSamples(samples)
        , mMultiview(getMultiview())
        , mMultiviewFbo{ new osg::FrameBufferObject }
        , mLayerFbo{ new osg::FrameBufferObject, new osg::FrameBufferObject }
        , mLayerMsaaFbo{ new osg::FrameBufferObject, new osg::FrameBufferObject }
    {
    }

    MultiviewFramebuffer::~MultiviewFramebuffer()
    {
    }

    void MultiviewFramebuffer::attachColorComponent(GLint sourceFormat, GLint sourceType, GLint internalFormat)
    {
        if (mMultiview)
        {
#ifdef OSG_HAS_MULTIVIEW
            mMultiviewColorTexture = createTextureArray(sourceFormat, sourceType, internalFormat);
            mMultiviewFbo->setAttachment(osg::Camera::COLOR_BUFFER, osg::FrameBufferAttachment(mMultiviewColorTexture, osg::Camera::FACE_CONTROLLED_BY_MULTIVIEW_SHADER, 0));
            for (unsigned i = 0; i < 2; i++)
            {
                mColorTexture[i] = createTextureView_Texture2DFromTexture2DArray(mMultiviewColorTexture.get(), i);
                mLayerFbo[i]->setAttachment(osg::Camera::COLOR_BUFFER, osg::FrameBufferAttachment(mColorTexture[i]));
            }
#endif
        }
        else
        {
            for (unsigned i = 0; i < 2; i++)
            {
                if (mSamples > 1)
                {
                    mMsaaColorTexture[i] = createTextureMsaa(sourceFormat, sourceType, internalFormat);
                    mLayerMsaaFbo[i]->setAttachment(osg::Camera::COLOR_BUFFER, osg::FrameBufferAttachment(mMsaaColorTexture[i]));
                }
                mColorTexture[i] = createTexture(sourceFormat, sourceType, internalFormat);
                mLayerFbo[i]->setAttachment(osg::Camera::COLOR_BUFFER, osg::FrameBufferAttachment(mColorTexture[i]));
            }
        }
    }

    void MultiviewFramebuffer::attachDepthComponent(GLint sourceFormat, GLint sourceType, GLint internalFormat)
    {
        if (mMultiview)
        {
#ifdef OSG_HAS_MULTIVIEW
            mMultiviewDepthTexture = createTextureArray(sourceFormat, sourceType, internalFormat);
            mMultiviewFbo->setAttachment(osg::Camera::PACKED_DEPTH_STENCIL_BUFFER, osg::FrameBufferAttachment(mMultiviewDepthTexture, osg::Camera::FACE_CONTROLLED_BY_MULTIVIEW_SHADER, 0));
            for (unsigned i = 0; i < 2; i++)
            {
                mDepthTexture[i] = createTextureView_Texture2DFromTexture2DArray(mMultiviewDepthTexture.get(), i);
                mLayerFbo[i]->setAttachment(osg::Camera::PACKED_DEPTH_STENCIL_BUFFER, osg::FrameBufferAttachment(mDepthTexture[i]));
            }
#endif
        }
        else
        {
            for (unsigned i = 0; i < 2; i++)
            {
                if (mSamples > 1)
                {
                    mMsaaDepthTexture[i] = createTextureMsaa(sourceFormat, sourceType, internalFormat);
                    mLayerMsaaFbo[i]->setAttachment(osg::Camera::PACKED_DEPTH_STENCIL_BUFFER, osg::FrameBufferAttachment(mMsaaDepthTexture[i]));
                }
                mDepthTexture[i] = createTexture(sourceFormat, sourceType, internalFormat);
                mLayerFbo[i]->setAttachment(osg::Camera::PACKED_DEPTH_STENCIL_BUFFER, osg::FrameBufferAttachment(mDepthTexture[i]));
            }
        }
    }

    osg::FrameBufferObject* MultiviewFramebuffer::multiviewFbo()
    {
        return mMultiviewFbo;
    }

    osg::FrameBufferObject* MultiviewFramebuffer::layerFbo(int i)
    {
        return mLayerFbo[i];
    }

    osg::FrameBufferObject* MultiviewFramebuffer::layerMsaaFbo(int i)
    {
        return mLayerMsaaFbo[i];
    }

    osg::Texture2DArray* MultiviewFramebuffer::multiviewColorBuffer()
    {
        return mMultiviewColorTexture;
    }

    osg::Texture2D* MultiviewFramebuffer::layerColorBuffer(int i)
    {
        return mColorTexture[i];
    }

    osg::Texture2D* MultiviewFramebuffer::layerDepthBuffer(int i)
    {
        return mDepthTexture[i];
    }
    void MultiviewFramebuffer::attachTo(osg::Camera* camera)
    {
#ifdef OSG_HAS_MULTIVIEW
        if (mMultiview)
        {
            if (mMultiviewColorTexture)
            {
                camera->attach(osg::Camera::COLOR_BUFFER, mMultiviewColorTexture, 0, osg::Camera::FACE_CONTROLLED_BY_MULTIVIEW_SHADER, false, mSamples);
                camera->getBufferAttachmentMap()[osg::Camera::COLOR_BUFFER]._internalFormat = mMultiviewColorTexture->getInternalFormat();
                camera->getBufferAttachmentMap()[osg::Camera::COLOR_BUFFER]._mipMapGeneration = false;
            }
            if (mMultiviewDepthTexture)
            {
                camera->attach(osg::Camera::PACKED_DEPTH_STENCIL_BUFFER, mMultiviewDepthTexture, 0, osg::Camera::FACE_CONTROLLED_BY_MULTIVIEW_SHADER, false, mSamples);
                camera->getBufferAttachmentMap()[osg::Camera::PACKED_DEPTH_STENCIL_BUFFER]._internalFormat = mMultiviewDepthTexture->getInternalFormat();
                camera->getBufferAttachmentMap()[osg::Camera::PACKED_DEPTH_STENCIL_BUFFER]._mipMapGeneration = false;
            }
        }
#endif
        camera->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);
        
        if (!mCullCallback)
            mCullCallback = new UpdateRenderStagesCallback(this);
        camera->addCullCallback(mCullCallback);
    }

    void MultiviewFramebuffer::detachFrom(osg::Camera* camera)
    {
#ifdef OSG_HAS_MULTIVIEW
        if (mMultiview)
        {
            if (mMultiviewColorTexture)
            {
                camera->detach(osg::Camera::COLOR_BUFFER);
            }
            if (mMultiviewDepthTexture)
            {
                camera->detach(osg::Camera::PACKED_DEPTH_STENCIL_BUFFER);
            }
        }
#endif
        camera->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER);
        if (mCullCallback)
            camera->removeCullCallback(mCullCallback);
    }

    osg::Texture2D* MultiviewFramebuffer::createTexture(GLint sourceFormat, GLint sourceType, GLint internalFormat)
    {
        osg::Texture2D* texture = new osg::Texture2D;
        texture->setTextureSize(mWidth, mHeight);
        texture->setSourceFormat(sourceFormat);
        texture->setSourceType(sourceType);
        texture->setInternalFormat(internalFormat);
        texture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
        texture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
        texture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
        texture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
        texture->setWrap(osg::Texture::WRAP_R, osg::Texture::CLAMP_TO_EDGE);
        return texture;
    }

    osg::Texture2DMultisample* MultiviewFramebuffer::createTextureMsaa(GLint sourceFormat, GLint sourceType, GLint internalFormat)
    {
        osg::Texture2DMultisample* texture = new osg::Texture2DMultisample;
        texture->setTextureSize(mWidth, mHeight);
        texture->setNumSamples(mSamples);
        texture->setSourceFormat(sourceFormat);
        texture->setSourceType(sourceType);
        texture->setInternalFormat(internalFormat);
        texture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
        texture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
        texture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
        texture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
        texture->setWrap(osg::Texture::WRAP_R, osg::Texture::CLAMP_TO_EDGE);
        return texture;
    }

    osg::Texture2DArray* MultiviewFramebuffer::createTextureArray(GLint sourceFormat, GLint sourceType, GLint internalFormat)
    {
        osg::Texture2DArray* textureArray = new osg::Texture2DArray;
        textureArray->setTextureSize(mWidth, mHeight, 2);
        textureArray->setSourceFormat(sourceFormat);
        textureArray->setSourceType(sourceType);
        textureArray->setInternalFormat(internalFormat);
        textureArray->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
        textureArray->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
        textureArray->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
        textureArray->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
        textureArray->setWrap(osg::Texture::WRAP_R, osg::Texture::CLAMP_TO_EDGE);
        return textureArray;
    }
}
