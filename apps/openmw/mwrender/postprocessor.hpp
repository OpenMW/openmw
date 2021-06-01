#ifndef OPENMW_MWRENDER_POSTPROCESSOR_H
#define OPENMW_MWRENDER_POSTPROCESSOR_H

#include <osg/ref_ptr>

namespace osg
{
    class Texture2D;
    class Group;
    class FrameBufferObject;
    class Camera;
}

namespace osgViewer
{
    class Viewer;
}

namespace MWRender
{
    class PostProcessor
    {
    public:
        PostProcessor(osgViewer::Viewer* viewer, osg::Group* rootNode);

        auto getMsaaFbo() { return mMsaaFbo; }
        auto getFbo() { return mFbo; }

        void resize(int width, int height);

    private:
        osgViewer::Viewer* mViewer;
        osg::ref_ptr<osg::Group> mRootNode;
        osg::ref_ptr<osg::Camera> mHUDCamera;

        osg::ref_ptr<osg::FrameBufferObject> mMsaaFbo;
        osg::ref_ptr<osg::FrameBufferObject> mFbo;

        osg::ref_ptr<osg::Texture2D> mSceneTex;
        osg::ref_ptr<osg::Texture2D> mDepthTex;

        void createTexturesAndCamera(int width, int height);
    };
}

#endif