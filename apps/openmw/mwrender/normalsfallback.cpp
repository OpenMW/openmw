#include "normalsfallback.hpp"

#include <osg/Texture2D>

#include <components/resource/resourcesystem.hpp>
#include <components/resource/scenemanager.hpp>
#include <components/sceneutil/rtt.hpp>
#include <components/sceneutil/shadow.hpp>
#include <components/shader/shadermanager.hpp>
#include <components/settings/values.hpp>
#include <components/debug/debuglog.hpp>
#include <components/stereo/multiview.hpp>

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"

#include "vismask.hpp"
#include "renderbin.hpp"
#include "camera.hpp"
#include "renderingmanager.hpp"
#include "postprocessor.hpp"

namespace MWRender
{

    CopyTextureCallback::CopyTextureCallback(osg::ref_ptr<PostProcessor> postProcessor, bool copyNormals)
        : mPostProcessor(postProcessor)
        , mCopyNormals(copyNormals)
    {
    }

    void CopyTextureCallback::drawImplementation(osg::RenderInfo& renderInfo, const osg::Drawable* drawable) const
    {
        osg::State& state = *renderInfo.getState();
        size_t frameId = state.getFrameStamp()->getFrameNumber() % 2;

        const auto& fbo = mPostProcessor->getFbo(PostProcessor::FBO_Primary, frameId);
        const auto& opaqueFbo = mPostProcessor->getFbo(PostProcessor::FBO_OpaqueDepth, frameId);

        state.applyTextureAttribute(0, mPostProcessor->getTexture(PostProcessor::Tex_Scene, frameId));

        if (mCopyNormals)
        {
            opaqueFbo->apply(state, osg::FrameBufferObject::DRAW_FRAMEBUFFER);

            glClearColor(0.0, 0.0, 0.0, 1.0);
            glClear(GL_COLOR_BUFFER_BIT);
        }

        drawable->drawImplementation(renderInfo);

        if (mCopyNormals)
            fbo->apply(state, osg::FrameBufferObject::DRAW_FRAMEBUFFER);
    }

    class NormalsFallbackCamera : public SceneUtil::RTTNode
    {
    public:
        NormalsFallbackCamera(osg::ref_ptr<PostProcessor> postProcessor, osg::Node* scene, unsigned int cullMask, osg::ref_ptr<osg::Texture> tex, osg::ref_ptr<osg::Texture> depthTex, int mode)
            : RTTNode(postProcessor->renderWidth(), postProcessor->renderHeight(), 0, false, 0, StereoAwareness::Aware, false)
        {
            setColorBufferInternalFormat(GL_RGB);
            setDepthBufferInternalFormat(GL_DEPTH24_STENCIL8);
            mPostProcessor = postProcessor;
            mScene = scene;
            mCullMask = cullMask;
            mColorTexture = tex;
            mDepthTexture = depthTex;
            mViewDistance = std::min(Settings::postProcessing().mCameraNormalsFallbackRenderingDistance, Settings::camera().mViewingDistance);
            mNormalsMode = mode;
        }

        void setDefaults(osg::Camera* camera) override
        {
            if (mNormalsMode == NormalsMode_PackedTextureRerender)
                camera->setReferenceFrame(osg::Camera::RELATIVE_RF);
            else
                camera->setReferenceFrame(osg::Camera::ABSOLUTE_RF);

            if (!Settings::shaders().mAutoUseTerrainNormalMaps)
                camera->setName((mNormalsMode == NormalsMode_PackedTextureRerender) ? "Normals Camera redraw singlelayer" : "Normals Camera singlelayer");
            else
                camera->setName((mNormalsMode == NormalsMode_PackedTextureRerender) ? "Normals Camera redraw" : "Normals Camera");

            camera->addChild(mScene);

            if (mNormalsMode == NormalsMode_PackedTextureRerender)
                camera->setRenderOrder(osg::Camera::POST_RENDER, -1);

            if (mNormalsMode == NormalsMode_PackedTextureRerender)
                camera->setClearMask(0);

            camera->setClearColor(osg::Vec4f(0.0, 0.0, 0.0, 1.0));
            camera->setCullMask(mCullMask);
            camera->setNodeMask(Mask_RenderToTexture);

            osg::Camera::CullingMode cullingMode;
            cullingMode = osg::Camera::DEFAULT_CULLING | osg::Camera::FAR_PLANE_CULLING;
            if (!Settings::camera().mSmallFeatureCulling)
                cullingMode &= ~(osg::CullStack::SMALL_FEATURE_CULLING);
            else
            {
                camera->setSmallFeatureCullingPixelSize(Settings::camera().mSmallFeatureCullingPixelSize);
                cullingMode |= osg::CullStack::SMALL_FEATURE_CULLING;
            }

            camera->setComputeNearFarMode(osg::Camera::DO_NOT_COMPUTE_NEAR_FAR);
            camera->setCullingMode(cullingMode);
            camera->setCullingActive(true);

            camera->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF | osg::StateAttribute::OVERRIDE);

            camera->attach(osg::Camera::COLOR_BUFFER, mColorTexture, 0, 0, false, 0);
            SceneUtil::attachAlphaToCoverageFriendlyFramebufferToCamera(camera, osg::Camera::COLOR_BUFFER, mColorTexture, 0, 0, false, false);
            camera->attach(osg::Camera::PACKED_DEPTH_STENCIL_BUFFER, mDepthTexture, 0, 0, false, 0);

            if (mNormalsMode != NormalsMode_PackedTextureRerender)
            {
                const double width = Settings::video().mResolutionX;
                const double height = Settings::video().mResolutionY;
                double aspect = (height == 0.0) ? 1.0 : width / height;

                camera->setProjectionMatrixAsPerspective(Settings::camera().mFieldOfView, aspect, Settings::camera().mNearClip, mViewDistance);

                if (getenv("PROJECTION") != nullptr)
                {
                    osg::Matrixf projectionMatrix;
                    if (SceneUtil::AutoDepth::isReversed())
                        projectionMatrix = static_cast<osg::Matrixf>(SceneUtil::getReversedZProjectionMatrixAsPerspective(
Settings::camera().mFieldOfView, aspect, Settings::camera().mNearClip, mViewDistance));
                    else
                        projectionMatrix = camera->getProjectionMatrix();

                    camera->getOrCreateStateSet()->addUniform(new osg::Uniform("projectionMatrix", static_cast<osg::Matrixf>(projectionMatrix)), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
                }
            }

            std::map<std::string, std::string> defineMap;
            Shader::ShaderManager& shaderMgr = MWBase::Environment::get().getResourceSystem()->getSceneManager()->getShaderManager();
            osg::ref_ptr<osg::Program> program = shaderMgr.getProgram("normal", defineMap);
            camera->getOrCreateStateSet()->setAttributeAndModes(program.get(), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
            camera->getOrCreateStateSet()->setDefine("NORMALS_ONLY", "1", osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
            SceneUtil::ShadowManager::instance().disableShadowsForStateSet(*camera->getOrCreateStateSet());
        }

        void apply(osg::Camera* camera) override
        {
            camera->setCullMask(mCullMask);

            if(mDirty)
            {
                camera->dirtyAttachmentMap();
                mDirty = false;
            }

            if (mNormalsMode != NormalsMode_PackedTextureRerender)
            {
                camera->setViewMatrix(MWBase::Environment::get().getWorld()->getCamera()->getViewMatrix());
            }
        }

        void setCullMask(unsigned int mask)
        {
            mCullMask = mask;
        }

        void update(size_t frameId)
        {
        }

        void dirty()
        {
  //          if (getenv("ONECAMERA") == nullptr)
                mDirty = true;
        }

    private:
        osg::ref_ptr<osg::Node> mScene;
        osg::ref_ptr<PostProcessor> mPostProcessor;
        osg::ref_ptr<osg::Texture> mColorTexture;
        osg::ref_ptr<osg::Texture> mDepthTexture;
        osg::Matrixf mProjectionMatrix;
        int mViewDistance;
        unsigned int mCullMask = 0;
        bool mDirty;
        int mNormalsMode;
    };


    osg::ref_ptr<osg::Geometry> NormalsFallback::createGeometry(bool normals)
    {
        Shader::ShaderManager& shaderMgr = MWBase::Environment::get().getResourceSystem()->getSceneManager()->getShaderManager();
        Shader::ShaderManager::DefineMap defines;

        osg::ref_ptr<osg::Depth> depth = new SceneUtil::AutoDepth;
        depth->setWriteMask(false);

        osg::ref_ptr<osg::Vec3Array> verts = new osg::Vec3Array;
        verts->push_back(osg::Vec3f(-1, -1, 0));
        verts->push_back(osg::Vec3f(-1, 3, 0));
        verts->push_back(osg::Vec3f(3, -1, 0));

        osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry;
        geometry->setUseDisplayList(false);
        geometry->setUseVertexBufferObjects(true);
        geometry->setVertexArray(verts);
  	geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::TRIANGLES, 0, 3));
        geometry->setCullingActive(false);

        osg::ref_ptr<osg::StateSet> stateSet = new osg::StateSet;
        stateSet->setRenderBinDetails(MWRender::RenderBin_DepthSorted - 2, "RenderBin", osg::StateSet::OVERRIDE_RENDERBIN_DETAILS);
        stateSet->setAttributeAndModes(shaderMgr.getProgram("fullscreen_tri", defines), osg::StateAttribute::ON);
        stateSet->setAttributeAndModes(new osg::BlendFunc, osg::StateAttribute::OFF);

        stateSet->setAttributeAndModes(depth, osg::StateAttribute::ON);
        stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
        stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
        stateSet->addUniform(new osg::Uniform("lastShader", 0), osg::StateAttribute::ON);
        stateSet->addUniform(new osg::Uniform("scaling", osg::Vec2f(1, 1)), osg::StateAttribute::ON);
        stateSet->setDefine("DECODE", normals ? "1" : "0", osg::StateAttribute::ON);

        geometry->setStateSet(stateSet);

        return geometry;
    }


    NormalsFallback::NormalsFallback(osg::Group* rootNode, osg::Group* sceneRoot, osg::ref_ptr<PostProcessor> postProcessor, int normalsMode)
        : mRootNode(rootNode)
        , mSceneRoot(sceneRoot)
        , mPostProcessor(postProcessor)
        , mNormalsMode(normalsMode)
    {
        unsigned int cullMask;

        mNormalsTex = new osg::Texture2D;
        Stereo::setMultiviewCompatibleTextureSize(mNormalsTex, postProcessor->renderWidth(), postProcessor->renderHeight());
        mNormalsTex->setSourceFormat(GL_RGB);
        mNormalsTex->setSourceType(GL_UNSIGNED_BYTE);
        mNormalsTex->setInternalFormat(GL_RGB);
        mNormalsTex->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture::LINEAR);
        mNormalsTex->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture::LINEAR);
        mNormalsTex->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
        mNormalsTex->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
        mNormalsTex->setResizeNonPowerOfTwoHint(false);
        mNormalsTex->dirtyTextureObject();

        if (mNormalsMode == NormalsMode_Camera)
            cullMask = Mask_Scene | Mask_Object | Mask_Static | Mask_Terrain | Mask_Actor | Mask_Player | Mask_Groundcover | Mask_Water;
        else
            cullMask = Mask_Scene | Mask_Terrain | Mask_Water | Mask_Player;

        for (int i = 0; i < 2; i++)
        {
            if (mNormalsMode == NormalsMode_PackedTextureRerender || mNormalsMode == NormalsMode_PackedTextureFetch)
            {
               mCopyTextureCallbacks[i] = new CopyTextureCallback(mPostProcessor, (i == 0) ? true : false);
               mNormalsFallbackGeometries[i] = createGeometry((i == 0) ? true : false);
               mNormalsFallbackGeometries[i]->setDrawCallback(mCopyTextureCallbacks[i]);
            }
        }

        if (mNormalsMode == NormalsMode_Camera || mNormalsMode == NormalsMode_PackedTextureRerender)
        {
            if (getenv("ONECAMERA") == nullptr)
            {
                mCameras[0] = new NormalsFallbackCamera(mPostProcessor, mSceneRoot, cullMask, mPostProcessor->getTexture(PostProcessor::Tex_Normal, 0), mPostProcessor->getTexture(PostProcessor::Tex_Depth, 0), mNormalsMode);
                mCameras[1] = new NormalsFallbackCamera(mPostProcessor, mSceneRoot, cullMask, mPostProcessor->getTexture(PostProcessor::Tex_Normal, 1), mPostProcessor->getTexture(PostProcessor::Tex_Depth, 1), mNormalsMode);
            }
            else
                mCameras[0] = new NormalsFallbackCamera(mPostProcessor, mSceneRoot, cullMask, mNormalsTex, mPostProcessor->getTexture(PostProcessor::Tex_Depth, 0), mNormalsMode);
        }
    }														


    NormalsFallback::~NormalsFallback()
    {

        disable();

        if (mCameras[0])
            mCameras[0] = nullptr;

        if (mCameras[1])
            mCameras[1] = nullptr;

        if (mNormalsFallbackGeometries[0])
            mNormalsFallbackGeometries[0] = nullptr;

        if (mNormalsFallbackGeometries[1])
            mNormalsFallbackGeometries[1] = nullptr;
    }


    void NormalsFallback::update(size_t frameId)
    {
        if (mCameras[0] && mCameras[1])
        {
            mCameras[frameId]->setNodeMask(~0u);
            mCameras[!frameId]->setNodeMask(0);

            mCameras[frameId]->update(frameId);
        }
    }

    void NormalsFallback::dirty()
    {
        if (mCameras[0])
            mCameras[0]->dirty();

        if (mCameras[1])
            mCameras[1]->dirty();
    }

    void NormalsFallback::enable()
    {
   //     Log(Debug::Error) << "NormalsFallback::enable";

        MWBase::Environment::get().getWorld()->getRenderingManager()->setNormalsFallbackDefines(true, mNormalsMode);

        if (mNormalsMode == NormalsMode_PackedTextureRerender)
            mPostProcessor->setPostPass(true);

        if (mNormalsFallbackGeometries[0])
        {
            mRootNode->addChild(mNormalsFallbackGeometries[0]);
            mRootNode->addChild(mNormalsFallbackGeometries[1]);
        }

        if (mCameras[0])
        {
            mRootNode->addChild(mCameras[0]);
            mCameras[0]->setNodeMask(~0u);
        }

        if (mCameras[1])
        {
            mRootNode->addChild(mCameras[1]);
            mCameras[1]->setNodeMask(~0u);
        }
    }

    void NormalsFallback::disable()
    {
 //       Log(Debug::Error) << "NormalsFallback::disable";

        MWBase::Environment::get().getWorld()->getRenderingManager()->setNormalsFallbackDefines(false, mNormalsMode);

        if (mNormalsMode == NormalsMode_PackedTextureRerender && !Settings::postProcessing().mTransparentPostpass)
            mPostProcessor->setPostPass(false);

        if (mNormalsFallbackGeometries[0])
        {
            mRootNode->removeChild(mNormalsFallbackGeometries[0]);
            mRootNode->removeChild(mNormalsFallbackGeometries[1]);
        }

        if (mCameras[0])
        {
            mRootNode->removeChild(mCameras[0]);
            mCameras[0]->setNodeMask(0);
        }

        if (mCameras[1])
        {
            mRootNode->removeChild(mCameras[1]);
            mCameras[1]->setNodeMask(0);
        }
   }
}


