#ifndef STEREO_MULTIVIEW_H
#define STEREO_MULTIVIEW_H

#include <osg/ref_ptr>
#include <osg/GL>
#include <osg/Camera>

#include <array>
#include <memory>

namespace osg
{
    class FrameBufferObject;
    class Texture;
    class Texture2D;
    class Texture2DMultisample;
    class Texture2DArray;
}

namespace Stereo
{
    class UpdateRenderStagesCallback;

    //! Check if TextureView is supported. Results are undefined if called before configureExtensions().
    bool getTextureViewSupported();

    //! Check if Multiview should be used. Results are undefined if called before configureExtensions().
    bool getMultiview();

    //! Use the provided context to check what extensions are supported and configure use of multiview based on extensions and settings.
    void configureExtensions(unsigned int contextID);

    //! Sets the appropriate vertex buffer hint on OSG's display settings if needed
    void setVertexBufferHint();

    //! Creates a Texture2D as a texture view into a Texture2DArray
    osg::ref_ptr<osg::Texture2D> createTextureView_Texture2DFromTexture2DArray(osg::Texture2DArray* textureArray, int layer);

    //! Class that manages the specifics of GL_OVR_Multiview aware framebuffers, separating the layers into separate framebuffers, and disabling
    class MultiviewFramebuffer
    {
    public:
        MultiviewFramebuffer(int width, int height, int samples);
        ~MultiviewFramebuffer();

        void attachColorComponent(GLint sourceFormat, GLint sourceType, GLint internalFormat);
        void attachDepthComponent(GLint sourceFormat, GLint sourceType, GLint internalFormat);

        osg::FrameBufferObject* multiviewFbo();
        osg::FrameBufferObject* layerFbo(int i);
        osg::FrameBufferObject* layerMsaaFbo(int i);
        osg::Texture2DArray* multiviewColorBuffer();
        osg::Texture2D* layerColorBuffer(int i);
        osg::Texture2D* layerDepthBuffer(int i);

        void attachTo(osg::Camera* camera);
        void detachFrom(osg::Camera* camera);

        int width() const { return mWidth; }
        int height() const { return mHeight; }
        int samples() const { return mSamples; };

    private:
        osg::Texture2D* createTexture(GLint sourceFormat, GLint sourceType, GLint internalFormat);
        osg::Texture2DMultisample* createTextureMsaa(GLint sourceFormat, GLint sourceType, GLint internalFormat);
        osg::Texture2DArray* createTextureArray(GLint sourceFormat, GLint sourceType, GLint internalFormat);

        int mWidth;
        int mHeight;
        int mSamples;
        bool mMultiview;
        osg::ref_ptr<UpdateRenderStagesCallback> mCullCallback;
        osg::ref_ptr<osg::FrameBufferObject> mMultiviewFbo;
        std::array<osg::ref_ptr<osg::FrameBufferObject>, 2> mLayerFbo;
        std::array<osg::ref_ptr<osg::FrameBufferObject>, 2> mLayerMsaaFbo;
        osg::ref_ptr<osg::Texture2DArray> mMultiviewColorTexture;
        osg::ref_ptr<osg::Texture2DArray> mMultiviewDepthTexture;
        std::array<osg::ref_ptr<osg::Texture2D>, 2> mColorTexture;
        std::array<osg::ref_ptr<osg::Texture2DMultisample>, 2> mMsaaColorTexture;
        std::array<osg::ref_ptr<osg::Texture2D>, 2> mDepthTexture;
        std::array<osg::ref_ptr<osg::Texture2DMultisample>, 2> mMsaaDepthTexture;
    };
}

#endif
