#ifndef OPENMW_MWRENDER_PINGPONGCULL_H
#define OPENMW_MWRENDER_PINGPONGCULL_H

#include <array>

#include <components/sceneutil/nodecallback.hpp>

#include "postprocessor.hpp"

namespace osg
{
    class StateSet;
    class Viewport;
}

namespace MWRender
{
    class PostProcessor;
    class PingPongCull : public SceneUtil::NodeCallback<PingPongCull, osg::Node*, osgUtil::CullVisitor*>
    {
    public:
        PingPongCull(PostProcessor* pp);
        ~PingPongCull();

        void operator()(osg::Node* node, osgUtil::CullVisitor* nv);
    private:
        std::array<osg::Matrixf, 2> mLastViewMatrix;
        osg::ref_ptr<osg::StateSet> mViewportStateset;
        osg::ref_ptr<osg::Viewport> mViewport;
        PostProcessor* mPostProcessor;
    };
}

#endif
