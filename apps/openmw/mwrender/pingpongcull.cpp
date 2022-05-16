#include "pingpongcull.hpp"

#include <osg/Camera>
#include <osg/FrameBufferObject>
#include <osgUtil/CullVisitor>

#include "postprocessor.hpp"
#include "pingpongcanvas.hpp"

namespace MWRender
{
    void PingPongCull::operator()(osg::Node* node, osgUtil::CullVisitor* cv)
    {
        osgUtil::RenderStage* renderStage = cv->getCurrentRenderStage();
        size_t frame = cv->getTraversalNumber();
        size_t frameId = frame % 2;

        MWRender::PostProcessor* postProcessor = dynamic_cast<MWRender::PostProcessor*>(cv->getCurrentCamera()->getUserData());

        postProcessor->getStateUpdater()->setViewMatrix(cv->getCurrentCamera()->getViewMatrix());
        postProcessor->getStateUpdater()->setInvViewMatrix(cv->getCurrentCamera()->getInverseViewMatrix());
        postProcessor->getStateUpdater()->setPrevViewMatrix(mLastViewMatrix);
        mLastViewMatrix = cv->getCurrentCamera()->getViewMatrix();
        postProcessor->getStateUpdater()->setEyePos(cv->getEyePoint());
        postProcessor->getStateUpdater()->setEyeVec(cv->getLookVectorLocal());

        if (!postProcessor || !postProcessor->getFbo(PostProcessor::FBO_Primary, frameId))
        {
            renderStage->setMultisampleResolveFramebufferObject(nullptr);
            renderStage->setFrameBufferObject(nullptr);
            traverse(node, cv);
            return;
        }

        if (!postProcessor->getFbo(PostProcessor::FBO_Multisample, frameId))
        {
            renderStage->setFrameBufferObject(postProcessor->getFbo(PostProcessor::FBO_Primary, frameId));
        }
        else
        {
            renderStage->setMultisampleResolveFramebufferObject(postProcessor->getFbo(PostProcessor::FBO_Primary, frameId));
            renderStage->setFrameBufferObject(postProcessor->getFbo(PostProcessor::FBO_Multisample, frameId));
        }

        traverse(node, cv);
    }
}
