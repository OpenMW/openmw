#include "pingpongcull.hpp"

#include <osg/Camera>
#include <osg/FrameBufferObject>
#include <osg/Texture2DArray>
#include <osg/Texture>
#include <osgUtil/CullVisitor>

#include <components/stereo/multiview.hpp>
#include <components/stereo/stereomanager.hpp>

#include "postprocessor.hpp"

namespace MWRender
{

    PingPongCull::PingPongCull(PostProcessor* pp)
        : mViewportStateset(nullptr)
        , mPostProcessor(pp)
    {
        if (Stereo::getStereo())
        {
            mViewportStateset = new osg::StateSet();
            mViewport = new osg::Viewport;
            mViewportStateset->setAttribute(mViewport);
        }
    }

    PingPongCull::~PingPongCull()
    {
        // Instantiate osg::ref_ptr<> destructor
    }

    void PingPongCull::operator()(osg::Node* node, osgUtil::CullVisitor* cv)
    {
        osgUtil::RenderStage* renderStage = cv->getCurrentRenderStage();
        size_t frame = cv->getTraversalNumber();
        size_t frameId = frame % 2;

        if (Stereo::getStereo())
        {
            auto& sm = Stereo::Manager::instance();
            auto view = sm.getEye(cv);
            int index = view == Stereo::Eye::Right ? 1 : 0;
            auto projectionMatrix = sm.computeEyeProjection(index, true);
            mPostProcessor->getStateUpdater()->setProjectionMatrix(projectionMatrix);
        }

        mPostProcessor->getStateUpdater()->setViewMatrix(cv->getCurrentCamera()->getViewMatrix());
        mPostProcessor->getStateUpdater()->setPrevViewMatrix(mLastViewMatrix[0]);
        mLastViewMatrix[0] = cv->getCurrentCamera()->getViewMatrix();

        mPostProcessor->getStateUpdater()->setEyePos(cv->getEyePoint());
        mPostProcessor->getStateUpdater()->setEyeVec(cv->getLookVectorLocal());

        if (!mPostProcessor->getFbo(PostProcessor::FBO_Multisample, frameId))
        {
            renderStage->setFrameBufferObject(mPostProcessor->getFbo(PostProcessor::FBO_Primary, frameId));
        }
        else
        {
            renderStage->setMultisampleResolveFramebufferObject(
                mPostProcessor->getFbo(PostProcessor::FBO_Primary, frameId));
            renderStage->setFrameBufferObject(mPostProcessor->getFbo(PostProcessor::FBO_Multisample, frameId));

            // The MultiView patch has a bug where it does not update resolve layers if the resolve framebuffer is
            // changed. So we do blit manually in this case
            if (Stereo::getMultiview() && !renderStage->getDrawCallback())
                Stereo::setMultiviewMSAAResolveCallback(renderStage);
        }

        if (mViewportStateset)
        {
            mViewport->setViewport(0, 0, mPostProcessor->renderWidth(), mPostProcessor->renderHeight());
            renderStage->setViewport(mViewport);
            cv->pushStateSet(mViewportStateset.get());
            traverse(node, cv);
            cv->popStateSet();
        }
        else
            traverse(node, cv);
    }
}
