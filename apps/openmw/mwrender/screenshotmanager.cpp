#include "screenshotmanager.hpp"

#include <condition_variable>
#include <mutex>

#include <osg/ImageUtils>
#include <osg/ShapeDrawable>
#include <osg/Texture2D>
#include <osg/TextureCubeMap>

#include <components/misc/stringops.hpp>
#include <components/resource/resourcesystem.hpp>
#include <components/resource/scenemanager.hpp>
#include <components/shader/shadermanager.hpp>
#include <components/sceneutil/util.hpp>

#include <components/settings/settings.hpp>

#include "../mwgui/loadingscreen.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"

#include "util.hpp"
#include "vismask.hpp"
#include "water.hpp"
#include "postprocessor.hpp"

namespace MWRender
{
    enum Screenshot360Type
    {
        Spherical,
        Cylindrical,
        Planet,
        RawCubemap
    };

    class NotifyDrawCompletedCallback : public osg::Camera::DrawCallback
    {
    public:
        NotifyDrawCompletedCallback()
            : mDone(false), mFrame(0)
        {
        }

        void operator () (osg::RenderInfo& renderInfo) const override
        {
            std::lock_guard<std::mutex> lock(mMutex);
            if (renderInfo.getState()->getFrameStamp()->getFrameNumber() >= mFrame && !mDone)
            {
                mDone = true;
                mCondition.notify_one();
            }
        }

        void waitTillDone()
        {
            std::unique_lock<std::mutex> lock(mMutex);
            if (mDone)
                return;
            mCondition.wait(lock);
        }

        void reset(unsigned int frame)
        {
            std::lock_guard<std::mutex> lock(mMutex);
            mDone = false;
            mFrame = frame;
        }

        mutable std::condition_variable mCondition;
        mutable std::mutex mMutex;
        mutable bool mDone;
        unsigned int mFrame;
    };

    class ReadImageFromFramebufferCallback : public osg::Drawable::DrawCallback
    {
    public:
        ReadImageFromFramebufferCallback(osg::Image* image, int width, int height)
            : mWidth(width), mHeight(height), mImage(image)
        {
        }
        void drawImplementation(osg::RenderInfo& renderInfo,const osg::Drawable* /*drawable*/) const override
        {
            int screenW = renderInfo.getCurrentCamera()->getViewport()->width();
            int screenH = renderInfo.getCurrentCamera()->getViewport()->height();
            double imageaspect = (double)mWidth/(double)mHeight;
            int leftPadding = std::max(0, static_cast<int>(screenW - screenH * imageaspect) / 2);
            int topPadding = std::max(0, static_cast<int>(screenH - screenW / imageaspect) / 2);
            int width = screenW - leftPadding*2;
            int height = screenH - topPadding*2;

            // Ensure we are reading from the resolved framebuffer and not the multisampled render buffer when in use.
            // glReadPixel() cannot read from multisampled targets.
            PostProcessor* postProcessor = dynamic_cast<PostProcessor*>(renderInfo.getCurrentCamera()->getUserData());

            if (postProcessor && postProcessor->getFbo() && postProcessor->getMsaaFbo())
            {
                osg::GLExtensions* ext = osg::GLExtensions::Get(renderInfo.getContextID(), false);
                if (ext)
                    ext->glBindFramebuffer(GL_FRAMEBUFFER_EXT, postProcessor->getFbo()->getHandle(renderInfo.getContextID()));
            }

            mImage->readPixels(leftPadding, topPadding, width, height, GL_RGB, GL_UNSIGNED_BYTE);
            mImage->scaleImage(mWidth, mHeight, 1);
        }
    private:
        int mWidth;
        int mHeight;
        osg::ref_ptr<osg::Image> mImage;
    };

    ScreenshotManager::ScreenshotManager(osgViewer::Viewer* viewer, osg::ref_ptr<osg::Group> rootNode, osg::ref_ptr<osg::Group> sceneRoot, Resource::ResourceSystem* resourceSystem, Water* water)
        : mViewer(viewer)
        , mRootNode(rootNode)
        , mSceneRoot(sceneRoot)
        , mDrawCompleteCallback(new NotifyDrawCompletedCallback)
        , mResourceSystem(resourceSystem)
        , mWater(water)
    {
    }

    ScreenshotManager::~ScreenshotManager()
    {
    }

    void ScreenshotManager::screenshot(osg::Image* image, int w, int h)
    {
        osg::Camera* camera = mViewer->getCamera();
        osg::ref_ptr<osg::Drawable> tempDrw = new osg::Drawable;
        tempDrw->setDrawCallback(new ReadImageFromFramebufferCallback(image, w, h));
        tempDrw->setCullingActive(false);
        tempDrw->getOrCreateStateSet()->setRenderBinDetails(100, "RenderBin", osg::StateSet::USE_RENDERBIN_DETAILS); // so its after all scene bins but before POST_RENDER gui camera
        camera->addChild(tempDrw);
        traversalsAndWait(mViewer->getFrameStamp()->getFrameNumber());
        // now that we've "used up" the current frame, get a fresh frame number for the next frame() following after the screenshot is completed
        mViewer->advance(mViewer->getFrameStamp()->getSimulationTime());
        camera->removeChild(tempDrw);
    }

    bool ScreenshotManager::screenshot360(osg::Image* image)
    {
        int screenshotW = mViewer->getCamera()->getViewport()->width();
        int screenshotH = mViewer->getCamera()->getViewport()->height();
        Screenshot360Type screenshotMapping = Spherical;

        const std::string& settingStr = Settings::Manager::getString("screenshot type", "Video");
        std::vector<std::string> settingArgs;
        Misc::StringUtils::split(settingStr, settingArgs);

        if (settingArgs.size() > 0)
        {
            std::string typeStrings[4] = {"spherical", "cylindrical", "planet", "cubemap"};
            bool found = false;

            for (int i = 0; i < 4; ++i)
            {
                if (settingArgs[0].compare(typeStrings[i]) == 0)
                {
                    screenshotMapping = static_cast<Screenshot360Type>(i);
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                Log(Debug::Warning) << "Wrong screenshot type: " << settingArgs[0] << ".";
                return false;
            }
        }

        // planet mapping needs higher resolution
        int cubeSize = screenshotMapping == Planet ? screenshotW : screenshotW / 2;

        if (settingArgs.size() > 1)
            screenshotW = std::min(10000, std::atoi(settingArgs[1].c_str()));

        if (settingArgs.size() > 2)
            screenshotH = std::min(10000, std::atoi(settingArgs[2].c_str()));

        if (settingArgs.size() > 3)
            cubeSize = std::min(5000, std::atoi(settingArgs[3].c_str()));

        bool rawCubemap = screenshotMapping == RawCubemap;

        if (rawCubemap)
            screenshotW = cubeSize * 6;  // the image will consist of 6 cube sides in a row
        else if (screenshotMapping == Planet)
            screenshotH = screenshotW;   // use square resolution for planet mapping

        std::vector<osg::ref_ptr<osg::Image>> images;
        images.reserve(6);

        for (int i = 0; i < 6; ++i)
            images.push_back(new osg::Image);

        osg::Vec3 directions[6] = {
            rawCubemap ? osg::Vec3(1,0,0) : osg::Vec3(0,0,1),
            osg::Vec3(0,0,-1),
            osg::Vec3(-1,0,0),
            rawCubemap ? osg::Vec3(0,0,1) : osg::Vec3(1,0,0),
            osg::Vec3(0,1,0),
            osg::Vec3(0,-1,0)};

        double rotations[] = {
            -osg::PI / 2.0,
            osg::PI / 2.0,
            osg::PI,
            0,
            osg::PI / 2.0,
            osg::PI / 2.0 };

        for (int i = 0; i < 6; ++i) // for each cubemap side
        {
            osg::Matrixd transform = osg::Matrixd::rotate(osg::Vec3(0,0,-1), directions[i]);

            if (!rawCubemap)
                transform *= osg::Matrixd::rotate(rotations[i],osg::Vec3(0,0,-1));

            osg::Image *sideImage = images[i].get();
            makeCubemapScreenshot(sideImage, cubeSize, cubeSize, transform);

            if (!rawCubemap)
                sideImage->flipHorizontal();
        }

        if (rawCubemap) // for raw cubemap don't run on GPU, just merge the images
        {
            image->allocateImage(cubeSize * 6,cubeSize,images[0]->r(),images[0]->getPixelFormat(),images[0]->getDataType());

            for (int i = 0; i < 6; ++i)
                osg::copyImage(images[i].get(),0,0,0,images[i]->s(),images[i]->t(),images[i]->r(),image,i * cubeSize,0,0);

            return true;
        }

        // run on GPU now:
        osg::ref_ptr<osg::TextureCubeMap> cubeTexture (new osg::TextureCubeMap);
        cubeTexture->setResizeNonPowerOfTwoHint(false);

        cubeTexture->setFilter(osg::Texture::MIN_FILTER,osg::Texture::NEAREST);
        cubeTexture->setFilter(osg::Texture::MAG_FILTER,osg::Texture::NEAREST);

        cubeTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
        cubeTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);

        for (int i = 0; i < 6; ++i)
            cubeTexture->setImage(i, images[i].get());

        osg::ref_ptr<osg::Camera> screenshotCamera(new osg::Camera);
        osg::ref_ptr<osg::ShapeDrawable> quad(new osg::ShapeDrawable(new osg::Box(osg::Vec3(0,0,0), 2.0)));

        std::map<std::string, std::string> defineMap;

        Shader::ShaderManager& shaderMgr = mResourceSystem->getSceneManager()->getShaderManager();
        osg::ref_ptr<osg::Shader> fragmentShader(shaderMgr.getShader("s360_fragment.glsl", defineMap,osg::Shader::FRAGMENT));
        osg::ref_ptr<osg::Shader> vertexShader(shaderMgr.getShader("s360_vertex.glsl", defineMap, osg::Shader::VERTEX));
        osg::ref_ptr<osg::StateSet> stateset = quad->getOrCreateStateSet();

        osg::ref_ptr<osg::Program> program(new osg::Program);
        program->addShader(fragmentShader);
        program->addShader(vertexShader);
        stateset->setAttributeAndModes(program, osg::StateAttribute::ON);

        stateset->addUniform(new osg::Uniform("cubeMap", 0));
        stateset->addUniform(new osg::Uniform("mapping", screenshotMapping));
        stateset->setTextureAttributeAndModes(0, cubeTexture, osg::StateAttribute::ON);

        screenshotCamera->addChild(quad);

        renderCameraToImage(screenshotCamera, image, screenshotW, screenshotH);

        return true;
    }

    void ScreenshotManager::traversalsAndWait(unsigned int frame)
    {
        // Ref https://gitlab.com/OpenMW/openmw/-/issues/6013
        mDrawCompleteCallback->reset(frame);
        mViewer->getCamera()->setFinalDrawCallback(mDrawCompleteCallback);

        mViewer->eventTraversal();
        mViewer->updateTraversal();
        mViewer->renderingTraversals();
        mDrawCompleteCallback->waitTillDone();
    }

    void ScreenshotManager::renderCameraToImage(osg::Camera *camera, osg::Image *image, int w, int h)
    {
        camera->setNodeMask(Mask_RenderToTexture);
        camera->attach(osg::Camera::COLOR_BUFFER, image);
        camera->setRenderOrder(osg::Camera::PRE_RENDER);
        camera->setReferenceFrame(osg::Camera::ABSOLUTE_RF);
        camera->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT,osg::Camera::PIXEL_BUFFER_RTT);
        camera->setViewport(0, 0, w, h);

        SceneUtil::setCameraClearDepth(camera);

        osg::ref_ptr<osg::Texture2D> texture (new osg::Texture2D);
        texture->setInternalFormat(GL_RGB);
        texture->setTextureSize(w,h);
        texture->setResizeNonPowerOfTwoHint(false);
        texture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
        texture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
        camera->attach(osg::Camera::COLOR_BUFFER,texture);

        image->setDataType(GL_UNSIGNED_BYTE);
        image->setPixelFormat(texture->getInternalFormat());

        mRootNode->addChild(camera);

        MWBase::Environment::get().getWindowManager()->getLoadingScreen()->loadingOn(false);

        // The draw needs to complete before we can copy back our image.
        traversalsAndWait(0);

        MWBase::Environment::get().getWindowManager()->getLoadingScreen()->loadingOff();

        // now that we've "used up" the current frame, get a fresh framenumber for the next frame() following after the screenshot is completed
        mViewer->advance(mViewer->getFrameStamp()->getSimulationTime());

        camera->removeChildren(0, camera->getNumChildren());
        mRootNode->removeChild(camera);
    }

    void ScreenshotManager::makeCubemapScreenshot(osg::Image *image, int w, int h, const osg::Matrixd& cameraTransform)
    {
        osg::ref_ptr<osg::Camera> rttCamera (new osg::Camera);
        float nearClip = Settings::Manager::getFloat("near clip", "Camera");
        float viewDistance = Settings::Manager::getFloat("viewing distance", "Camera");
        // each cubemap side sees 90 degrees
        if (SceneUtil::getReverseZ())
            rttCamera->setProjectionMatrix(SceneUtil::getReversedZProjectionMatrixAsPerspectiveInf(90.0, w/float(h), nearClip));
        else
            rttCamera->setProjectionMatrixAsPerspective(90.0, w/float(h), nearClip, viewDistance);
        rttCamera->setViewMatrix(mViewer->getCamera()->getViewMatrix() * cameraTransform);

        rttCamera->setUpdateCallback(new NoTraverseCallback);
        rttCamera->addChild(mSceneRoot);

        rttCamera->addChild(mWater->getReflectionNode());
        rttCamera->addChild(mWater->getRefractionNode());

        rttCamera->setCullMask(mViewer->getCamera()->getCullMask() & ~(Mask_GUI|Mask_FirstPerson));

        rttCamera->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        renderCameraToImage(rttCamera.get(),image,w,h);
    }
}
