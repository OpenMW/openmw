#ifndef OPENMW_MWRENDER_POSTPROCESSOR_H
#define OPENMW_MWRENDER_POSTPROCESSOR_H

#include <osg/Texture2D>
#include <osg/Group>
#include <osg/FrameBufferObject>
#include <osg/Camera>
#include <osg/ref_ptr>

namespace osgViewer
{
    class Viewer;
}

namespace MWRender
{
    class RenderingManager;

    class PostProcessor : public osg::Referenced
    {
    public:
        PostProcessor(RenderingManager& rendering, osgViewer::Viewer* viewer, osg::Group* rootNode);

        auto getMsaaFbo() { return mMsaaFbo; }
        auto getFbo() { return mFbo; }

        int getDepthFormat() { return mDepthFormat; }

        void resize(int width, int height, bool init=false);

    private:
        void createTexturesAndCamera(int width, int height);

        osgViewer::Viewer* mViewer;
        osg::ref_ptr<osg::Group> mRootNode;
        osg::ref_ptr<osg::Camera> mHUDCamera;

        osg::ref_ptr<osg::FrameBufferObject> mMsaaFbo;
        osg::ref_ptr<osg::FrameBufferObject> mFbo;

        osg::ref_ptr<osg::Texture2D> mSceneTex;
        osg::ref_ptr<osg::Texture2D> mDepthTex;

        int mDepthFormat;

        RenderingManager& mRendering;
    };
}

#endif