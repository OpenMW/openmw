#include "RenderStageEx"
#include "StateEx"

void RenderStageEx::drawInner(osg::RenderInfo& renderInfo,
                              osgUtil::RenderLeaf*& previous, bool& doCopyTexture)
{
#if 0
    // **************************************************************
    // Code from RenderStage class
    struct SubFunc
    {
        static void applyReadFBO(bool& apply_read_fbo,
                                 const osg::FrameBufferObject* read_fbo, osg::State& state)
        {
            if(read_fbo->isMultisample())
            {
                OSG_WARN << "Attempting to read from a"
                         " multisampled framebuffer object. Set a resolve"
                         " framebuffer on the RenderStage to fix this." << std::endl;
            }

            if(apply_read_fbo)
            {
                // Bind the monosampled FBO to read from
                read_fbo->apply(state, osg::FrameBufferObject::READ_FRAMEBUFFER);
                apply_read_fbo = false;
            }
        }
    };
    // **************************************************************
    // New code
    osg::State& state = *renderInfo.getState();
    //osg::FBOExtensions* fbo_ext = osg::FBOExtensions::instance(state.getContextID(), true);
    osg::GLExtensions* fbo_ext = state.get<osg::GLExtensions>();
    bool using_multiple_render_targets = false;

    if(fbo_ext)
    {
        if(_fbo.valid())
        {
            using_multiple_render_targets = _fbo->hasMultipleRenderingTargets();

            if(!using_multiple_render_targets)
            {
#if !defined(OSG_GLES1_AVAILABLE) && !defined(OSG_GLES2_AVAILABLE)

                if(getDrawBufferApplyMask())
                    glDrawBuffer(_drawBuffer);

                if(getReadBufferApplyMask())
                    glReadBuffer(_readBuffer);

#endif
            }
        }
        else
            fbo_ext->glBindFramebuffer(osg::FrameBufferObject::READ_DRAW_FRAMEBUFFER,
                                       static_cast<StateEx*>(&state)->getDefaultFbo());
    }

    RenderBin::draw(renderInfo, previous);

    // **************************************************************
    // Code from RenderStage class

    if(state.getCheckForGLErrors() != osg::State::NEVER_CHECK_GL_ERRORS)
    {
        if(state.checkGLErrors("after RenderBin::draw(..)"))
        {
            if(fbo_ext)
            {
                GLenum fbstatus = fbo_ext->glCheckFramebufferStatus(GL_FRAMEBUFFER_EXT);

                if(fbstatus != GL_FRAMEBUFFER_COMPLETE_EXT)
                {
                    OSG_NOTICE << "RenderStage::drawInner(,) FBO status = 0x" << std::hex <<
                               fbstatus << std::dec << std::endl;
                }
            }
        }
    }

    const osg::FrameBufferObject* read_fbo = fbo_ext ? _fbo.get() : 0;
    bool apply_read_fbo = false;

    if(fbo_ext && _resolveFbo.valid() && fbo_ext->glBlitFramebuffer)
    {
        GLbitfield blitMask = 0;
        bool needToBlitColorBuffers = false;

        //find which buffer types should be copied
        for(osg::FrameBufferObject::AttachmentMap::const_iterator
            it = _resolveFbo->getAttachmentMap().begin(),
            end = _resolveFbo->getAttachmentMap().end(); it != end; ++it)
        {
            switch(it->first)
            {
            case osg::Camera::DEPTH_BUFFER:
                blitMask |= GL_DEPTH_BUFFER_BIT;
                break;

            case osg::Camera::STENCIL_BUFFER:
                blitMask |= GL_STENCIL_BUFFER_BIT;
                break;

            case osg::Camera::PACKED_DEPTH_STENCIL_BUFFER:
                blitMask |= GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;
                break;

            case osg::Camera::COLOR_BUFFER:
                blitMask |= GL_COLOR_BUFFER_BIT;
                break;

            default:
                needToBlitColorBuffers = true;
                break;
            }
        }

        // Bind the resolve framebuffer to blit into.
        _fbo->apply(state, osg::FrameBufferObject::READ_FRAMEBUFFER);
        _resolveFbo->apply(state, osg::FrameBufferObject::DRAW_FRAMEBUFFER);

        if(blitMask)
        {
            // Blit to the resolve framebuffer.
            // Note that (with nvidia 175.16 windows drivers at least) if the read
            // framebuffer is multisampled then the dimension arguments are ignored
            // and the whole framebuffer is always copied.
            fbo_ext->glBlitFramebuffer(
                static_cast<GLint>(_viewport->x()), static_cast<GLint>(_viewport->y()),
                static_cast<GLint>(_viewport->x() + _viewport->width()),
                static_cast<GLint>(_viewport->y() + _viewport->height()),
                static_cast<GLint>(_viewport->x()), static_cast<GLint>(_viewport->y()),
                static_cast<GLint>(_viewport->x() + _viewport->width()),
                static_cast<GLint>(_viewport->y() + _viewport->height()),
                blitMask, GL_NEAREST);
        }

#if !defined(OSG_GLES1_AVAILABLE) && !defined(OSG_GLES2_AVAILABLE)

        if(needToBlitColorBuffers)
        {
            for(osg::FrameBufferObject::AttachmentMap::const_iterator
                it = _resolveFbo->getAttachmentMap().begin(),
                end = _resolveFbo->getAttachmentMap().end(); it != end; ++it)
            {
                osg::Camera::BufferComponent attachment = it->first;

                if(attachment >= osg::Camera::COLOR_BUFFER0)
                {
                    glReadBuffer(GL_COLOR_ATTACHMENT0_EXT + (attachment -
                                                             osg::Camera::COLOR_BUFFER0));
                    glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT + (attachment -
                                                             osg::Camera::COLOR_BUFFER0));
                    fbo_ext->glBlitFramebuffer(
                        static_cast<GLint>(_viewport->x()), static_cast<GLint>(_viewport->y()),
                        static_cast<GLint>(_viewport->x() + _viewport->width()),
                        static_cast<GLint>(_viewport->y() + _viewport->height()),
                        static_cast<GLint>(_viewport->x()), static_cast<GLint>(_viewport->y()),
                        static_cast<GLint>(_viewport->x() + _viewport->width()),
                        static_cast<GLint>(_viewport->y() + _viewport->height()),
                        GL_COLOR_BUFFER_BIT, GL_NEAREST);
                }
            }

            // reset the read and draw buffers?  will comment out for now with the assumption that
            // the buffers will be set explicitly when needed elsewhere.
            // glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
            // glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
        }

#endif
        apply_read_fbo = true;
        read_fbo = _resolveFbo.get();
        using_multiple_render_targets = read_fbo->hasMultipleRenderingTargets();
    }

    // now copy the rendered image to attached texture.
    if(doCopyTexture)
    {
        if(read_fbo) SubFunc::applyReadFBO(apply_read_fbo, read_fbo, state);

        copyTexture(renderInfo);
    }

    std::map< osg::Camera::BufferComponent, Attachment>::const_iterator itr;

    for(itr = _bufferAttachmentMap.begin();
        itr != _bufferAttachmentMap.end();
        ++itr)
    {
        if(itr->second._image.valid())
        {
            if(read_fbo) SubFunc::applyReadFBO(apply_read_fbo, read_fbo, state);

#if !defined(OSG_GLES1_AVAILABLE) && !defined(OSG_GLES2_AVAILABLE)

            if(using_multiple_render_targets)
            {
                int attachment = itr->first;

                if(attachment == osg::Camera::DEPTH_BUFFER
                   || attachment == osg::Camera::STENCIL_BUFFER)
                {
                    // assume first buffer rendered to is the one we want
                    glReadBuffer(read_fbo->getMultipleRenderingTargets()[0]);
                }
                else
                {
                    glReadBuffer(GL_COLOR_ATTACHMENT0_EXT + (attachment -
                                                             osg::Camera::COLOR_BUFFER0));
                }
            }
            else
            {
                if(_readBuffer != GL_NONE)
                {
                    glReadBuffer(_readBuffer);
                }
            }

#endif
            GLenum pixelFormat = itr->second._image->getPixelFormat();

            if(pixelFormat == 0) pixelFormat = _imageReadPixelFormat;

            if(pixelFormat == 0) pixelFormat = GL_RGB;

            GLenum dataType = itr->second._image->getDataType();

            if(dataType == 0) dataType = _imageReadPixelDataType;

            if(dataType == 0) dataType = GL_UNSIGNED_BYTE;

            itr->second._image->readPixels(static_cast<int>(_viewport->x()),
                                           static_cast<int>(_viewport->y()),
                                           static_cast<int>(_viewport->width()),
                                           static_cast<int>(_viewport->height()),
                                           pixelFormat, dataType);
        }
    }

    if(fbo_ext)
    {
        if(getDisableFboAfterRender())
        {
            // switch off the frame buffer object
            GLuint fboId = state.getGraphicsContext() ?
                           state.getGraphicsContext()->getDefaultFboId() : 0;
            fbo_ext->glBindFramebuffer(GL_FRAMEBUFFER_EXT, fboId);
        }

        doCopyTexture = true;
    }

    if(fbo_ext && _camera.valid())
    {
        // now generate mipmaps if they are required.
        const osg::Camera::BufferAttachmentMap& bufferAttachments =
            _camera->getBufferAttachmentMap();

        for(osg::Camera::BufferAttachmentMap::const_iterator itr =
                bufferAttachments.begin();
            itr != bufferAttachments.end();
            ++itr)
        {
            if(itr->second._texture.valid() && itr->second._mipMapGeneration)
            {
                state.setActiveTextureUnit(0);
                state.applyTextureAttribute(0, itr->second._texture.get());
                fbo_ext->glGenerateMipmap(itr->second._texture->getTextureTarget());
            }
        }
    }

#endif
}
