#include "postprocessor.hpp"

#include <osg/Group>
#include <osg/Camera>
#include <osg/Callback>
#include <osg/Texture2D>
#include <osg/FrameBufferObject>

#include <osgViewer/Viewer>

#include <components/settings/settings.hpp>

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
        CullCallback(MWRender::PostProcessor* postProcessor)
            : mPostProcessor(postProcessor)
            , mLastFrameNumber(0)
        {}

        void operator()(osg::Node* node, osg::NodeVisitor* nv) override
        {
            osgUtil::RenderStage* renderStage = nv->asCullVisitor()->getCurrentRenderStage();

            unsigned int frame = nv->getTraversalNumber();
            if (frame != mLastFrameNumber)
            {
                mLastFrameNumber = frame;
                if (!mPostProcessor->getMsaaFbo())
                {
                    renderStage->setFrameBufferObject(mPostProcessor->getFbo());
                }
                else
                {
                    renderStage->setMultisampleResolveFramebufferObject(mPostProcessor->getFbo());
                    renderStage->setFrameBufferObject(mPostProcessor->getMsaaFbo());
                }
            }

            traverse(node, nv);
        }
        MWRender::PostProcessor* mPostProcessor;
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
            mPostProcessor->resize(width, height);
        }

        MWRender::PostProcessor* mPostProcessor;
    };
}

namespace MWRender
{
    PostProcessor::PostProcessor(osgViewer::Viewer* viewer, osg::Group* rootNode)
        : mViewer(viewer)
        , mRootNode(new osg::Group)
    {
        int width = viewer->getCamera()->getViewport()->width();
        int height = viewer->getCamera()->getViewport()->height();

        createTexturesAndCamera(width, height);
        resize(width, height);

        mRootNode->addChild(mHUDCamera);
        mRootNode->addChild(rootNode);
        mViewer->setSceneData(mRootNode);

        // Main camera is treated specially, we need to manually set the FBO and
        // resolve FBO during the cull callback.
        mViewer->getCamera()->addCullCallback(new CullCallback(this));
        mViewer->getCamera()->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);
        mViewer->getCamera()->attach(osg::Camera::COLOR_BUFFER0, mSceneTex);
        mViewer->getCamera()->attach(osg::Camera::DEPTH_BUFFER, mDepthTex);

        mViewer->getCamera()->getGraphicsContext()->setResizedCallback(new ResizedCallback(this));
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

        mViewer->getCamera()->setUserData(mFbo);

        // When MSAA is enabled we must first render to a render buffer, then
        // blit the result to the FBO which is either passed to the main frame
        // buffer for display or used as the entry point for a post process chain.
        if (samples > 0)
        {
            mMsaaFbo = new osg::FrameBufferObject;
            osg::ref_ptr<osg::RenderBuffer> colorRB = new osg::RenderBuffer(width, height, mSceneTex->getInternalFormat(), samples);
            osg::ref_ptr<osg::RenderBuffer> depthRB = new osg::RenderBuffer(width, height, mDepthTex->getInternalFormat(), samples);
            mMsaaFbo->setAttachment(osg::Camera::COLOR_BUFFER0, osg::FrameBufferAttachment(colorRB));
            mMsaaFbo->setAttachment(osg::Camera::DEPTH_BUFFER, osg::FrameBufferAttachment(depthRB));
        }

        double prevWidth = mViewer->getCamera()->getViewport()->width();
        double prevHeight = mViewer->getCamera()->getViewport()->height();
        double scaleX = prevWidth / width;
        double scaleY = prevHeight / height;

        mViewer->getCamera()->resize(width,height);
        mHUDCamera->resize(width,height);

        mViewer->getCamera()->getProjectionMatrix() *= osg::Matrix::scale(scaleX, scaleY, 1.0);
    }

    void PostProcessor::createTexturesAndCamera(int width, int height)
    {
        mDepthTex = new osg::Texture2D;
        mDepthTex->setTextureSize(width, height);
        mDepthTex->setSourceFormat(GL_DEPTH_COMPONENT);
        mDepthTex->setSourceType(GL_FLOAT);
        mDepthTex->setInternalFormat(GL_DEPTH_COMPONENT32F);
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

        auto* stateset = mHUDCamera->getOrCreateStateSet();
        stateset->setTextureAttributeAndModes(0, mSceneTex, osg::StateAttribute::ON);
        stateset->setAttributeAndModes(program, osg::StateAttribute::ON);
        stateset->addUniform(new osg::Uniform("sceneTex", 0));
        stateset->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
        stateset->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
    }

}
