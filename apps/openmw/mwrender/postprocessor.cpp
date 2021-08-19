#include "postprocessor.hpp"

#include <osg/Group>
#include <osg/Camera>
#include <osg/Callback>
#include <osg/Texture2D>
#include <osg/FrameBufferObject>

#include <osgViewer/Viewer>

#include <components/settings/settings.hpp>
#include <components/sceneutil/util.hpp>
#include <components/debug/debuglog.hpp>

#include "vismask.hpp"
#include "renderingmanager.hpp"

namespace
{
    osg::ref_ptr<osg::Geometry> createFullScreenTri()
    {
        osg::ref_ptr<osg::Geometry> geom = new osg::Geometry;

        osg::ref_ptr<osg::Vec3Array> verts = new osg::Vec3Array;
        verts->push_back(osg::Vec3f(-1, -1, 0));
        verts->push_back(osg::Vec3f(-1, 3, 0));
        verts->push_back(osg::Vec3f(3, -1, 0));

        geom->setVertexArray(verts);

        geom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::TRIANGLES, 0, 3));

        return geom;
    }

    class CullCallback : public osg::NodeCallback
    {
    public:
        CullCallback()
            : mLastFrameNumber(0)
        {
        }

        void operator()(osg::Node* node, osg::NodeVisitor* nv) override
        {
            osgUtil::CullVisitor* cv = static_cast<osgUtil::CullVisitor*>(nv);
            osgUtil::RenderStage* renderStage = cv->getCurrentRenderStage();

            unsigned int frame = nv->getTraversalNumber();
            if (frame != mLastFrameNumber)
            {
                mLastFrameNumber = frame;

                MWRender::PostProcessor* postProcessor = dynamic_cast<MWRender::PostProcessor*>(cv->getCurrentCamera()->getUserData());

                if (!postProcessor)
                {
                    Log(Debug::Error) << "Failed retrieving user data for master camera: FBO setup failed";
                    traverse(node, nv);
                    return;
                }

                if (!postProcessor->getMsaaFbo())
                {
                    renderStage->setFrameBufferObject(postProcessor->getFbo());
                }
                else
                {
                    renderStage->setMultisampleResolveFramebufferObject(postProcessor->getFbo());
                    renderStage->setFrameBufferObject(postProcessor->getMsaaFbo());
                }
            }

            traverse(node, nv);
        }

    private:
        unsigned int mLastFrameNumber;
    };

    struct ResizedCallback : osg::GraphicsContext::ResizedCallback
    {
        ResizedCallback(MWRender::PostProcessor* postProcessor)
            : mPostProcessor(postProcessor)
        {
        }

        void resizedImplementation(osg::GraphicsContext* gc, int x, int y, int width, int height) override
        {
            gc->resizedImplementation(x, y, width, height);
            mPostProcessor->resize(width, height);
        }

        MWRender::PostProcessor* mPostProcessor;
    };
}

namespace MWRender
{
    PostProcessor::PostProcessor(RenderingManager& rendering, osgViewer::Viewer* viewer, osg::Group* rootNode)
        : mViewer(viewer)
        , mRootNode(new osg::Group)
        , mDepthFormat(GL_DEPTH_COMPONENT24)
        , mRendering(rendering)
    {
        if (!SceneUtil::getReverseZ())
            return;

        osg::GraphicsContext* gc = viewer->getCamera()->getGraphicsContext();
        unsigned int contextID = gc->getState()->getContextID();
        osg::GLExtensions* ext = gc->getState()->get<osg::GLExtensions>();

        constexpr char errPreamble[] = "Postprocessing and floating point depth buffers disabled: ";

        if (!ext->isFrameBufferObjectSupported)
        {
            Log(Debug::Warning) << errPreamble << "FrameBufferObject unsupported.";
            return;
        }

        if (Settings::Manager::getInt("antialiasing", "Video") > 1 && !ext->isRenderbufferMultisampleSupported())
        {
            Log(Debug::Warning) << errPreamble << "RenderBufferMultiSample unsupported. Disabling antialiasing will resolve this issue.";
            return;
        }

        if (osg::isGLExtensionSupported(contextID, "GL_ARB_depth_buffer_float"))
            mDepthFormat = GL_DEPTH_COMPONENT32F;
        else if (osg::isGLExtensionSupported(contextID, "GL_NV_depth_buffer_float"))
            mDepthFormat = GL_DEPTH_COMPONENT32F_NV;
        else
        {
            // TODO: Once we have post-processing implemented we want to skip this return and continue with setup.
            // Rendering to a FBO to fullscreen geometry has overhead (especially when MSAA is enabled) and there are no
            // benefits if no floating point depth formats are supported.
            Log(Debug::Warning) << errPreamble << "'GL_ARB_depth_buffer_float' and 'GL_NV_depth_buffer_float' unsupported.";
            return;
        }

        int width = viewer->getCamera()->getViewport()->width();
        int height = viewer->getCamera()->getViewport()->height();

        createTexturesAndCamera(width, height);
        resize(width, height);

        mRootNode->addChild(mHUDCamera);
        mRootNode->addChild(rootNode);
        mViewer->setSceneData(mRootNode);

        // We need to manually set the FBO and resolve FBO during the cull callback. If we were using a separate
        // RTT camera this would not be needed.
        mViewer->getCamera()->addCullCallback(new CullCallback);
        mViewer->getCamera()->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);
        mViewer->getCamera()->attach(osg::Camera::COLOR_BUFFER0, mSceneTex);
        mViewer->getCamera()->attach(osg::Camera::DEPTH_BUFFER, mDepthTex);

        mViewer->getCamera()->getGraphicsContext()->setResizedCallback(new ResizedCallback(this));
        mViewer->getCamera()->setUserData(this);
    }

    void PostProcessor::resize(int width, int height)
    {
        mDepthTex->setTextureSize(width, height);
        mSceneTex->setTextureSize(width, height);
        mDepthTex->dirtyTextureObject();
        mSceneTex->dirtyTextureObject();

        int samples = Settings::Manager::getInt("antialiasing", "Video");

        mFbo = new osg::FrameBufferObject;
        mFbo->setAttachment(osg::Camera::COLOR_BUFFER0, osg::FrameBufferAttachment(mSceneTex));
        mFbo->setAttachment(osg::Camera::DEPTH_BUFFER, osg::FrameBufferAttachment(mDepthTex));

        // When MSAA is enabled we must first render to a render buffer, then
        // blit the result to the FBO which is either passed to the main frame
        // buffer for display or used as the entry point for a post process chain.
        if (samples > 1)
        {
            mMsaaFbo = new osg::FrameBufferObject;
            osg::ref_ptr<osg::RenderBuffer> colorRB = new osg::RenderBuffer(width, height, mSceneTex->getInternalFormat(), samples);
            osg::ref_ptr<osg::RenderBuffer> depthRB = new osg::RenderBuffer(width, height, mDepthTex->getInternalFormat(), samples);
            mMsaaFbo->setAttachment(osg::Camera::COLOR_BUFFER0, osg::FrameBufferAttachment(colorRB));
            mMsaaFbo->setAttachment(osg::Camera::DEPTH_BUFFER, osg::FrameBufferAttachment(depthRB));
        }

        mViewer->getCamera()->resize(width, height);
        mHUDCamera->resize(width, height);
        mRendering.updateProjectionMatrix();
    }

    void PostProcessor::createTexturesAndCamera(int width, int height)
    {
        mDepthTex = new osg::Texture2D;
        mDepthTex->setTextureSize(width, height);
        mDepthTex->setSourceFormat(GL_DEPTH_COMPONENT);
        mDepthTex->setSourceType(SceneUtil::isFloatingPointDepthFormat(getDepthFormat()) ? GL_FLOAT : GL_UNSIGNED_INT);
        mDepthTex->setInternalFormat(mDepthFormat);
        mDepthTex->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::NEAREST);
        mDepthTex->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::NEAREST);
        mDepthTex->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
        mDepthTex->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
        mDepthTex->setResizeNonPowerOfTwoHint(false);

        mSceneTex = new osg::Texture2D;
        mSceneTex->setTextureSize(width, height);
        mSceneTex->setSourceFormat(GL_RGB);
        mSceneTex->setSourceType(GL_UNSIGNED_BYTE);
        mSceneTex->setInternalFormat(GL_RGB);
        mSceneTex->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::NEAREST);
        mSceneTex->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::NEAREST);
        mSceneTex->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
        mSceneTex->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
        mSceneTex->setResizeNonPowerOfTwoHint(false);

        mHUDCamera = new osg::Camera;
        mHUDCamera->setReferenceFrame(osg::Camera::ABSOLUTE_RF);
        mHUDCamera->setRenderOrder(osg::Camera::POST_RENDER);
        mHUDCamera->setClearColor(osg::Vec4(0.45, 0.45, 0.14, 1.0));
        mHUDCamera->setProjectionMatrix(osg::Matrix::ortho2D(0, 1, 0, 1));
        mHUDCamera->setAllowEventFocus(false);
        mHUDCamera->setViewport(0, 0, width, height);

        // Shaders calculate correct UV coordinates for our fullscreen triangle
        constexpr char vertSrc[] = R"GLSL(
            #version 120

            varying vec2 uv;

            void main()
            {
                gl_Position = vec4(gl_Vertex.xy, 0.0, 1.0);
                uv = gl_Position.xy * 0.5 + 0.5;
            }
        )GLSL";

        constexpr char fragSrc[] = R"GLSL(
            #version 120

            varying vec2 uv;
            uniform sampler2D sceneTex;

            void main()
            {
                gl_FragData[0] = texture2D(sceneTex, uv);
            }
        )GLSL";

        osg::ref_ptr<osg::Shader> vertShader = new osg::Shader(osg::Shader::VERTEX, vertSrc);
        osg::ref_ptr<osg::Shader> fragShader = new osg::Shader(osg::Shader::FRAGMENT, fragSrc);

        osg::ref_ptr<osg::Program> program = new osg::Program;
        program->addShader(vertShader);
        program->addShader(fragShader);

        mHUDCamera->addChild(createFullScreenTri());
        mHUDCamera->setNodeMask(Mask_RenderToTexture);

        auto* stateset = mHUDCamera->getOrCreateStateSet();
        stateset->setTextureAttributeAndModes(0, mSceneTex, osg::StateAttribute::ON);
        stateset->setAttributeAndModes(program, osg::StateAttribute::ON);
        stateset->addUniform(new osg::Uniform("sceneTex", 0));
        stateset->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
        stateset->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
    }

}
