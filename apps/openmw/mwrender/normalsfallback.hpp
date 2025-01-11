#ifndef OPENMW_MWRENDER_NORMALS_FALLBACK_H
#define OPENMW_MWRENDER_NORMALS_FALLBACK_H

#include <array>
#include <osg/ref_ptr>
#include <osg/Geometry>

namespace osg
{
    class Group;
    class FrameBufferObject;
}

namespace MWRender
{
    class PostProcessor;
    class NormalsFallbackCamera;

#define NormalsMode_MRT  0
#define NormalsMode_Camera 1
#define NormalsMode_PackedTextureRerender 2
#define NormalsMode_PackedTextureFetch 3
#define NormalsMode_PackedTextureFetchOnly 4

    class CopyTextureCallback : public osg::Drawable::DrawCallback
    {
    public:
        CopyTextureCallback(osg::ref_ptr<PostProcessor> postProcessor, bool copyNormals);

        void drawImplementation(osg::RenderInfo& renderInfo, const osg::Drawable* drawable) const override;

    private:
        osg::ref_ptr<PostProcessor> mPostProcessor;
        bool mCopyNormals;
    };

    // Postprocessing normals fallback
    class NormalsFallback
    {
    public:
        NormalsFallback(osg::Group* rootNode, osg::Group* sceneRoot, osg::ref_ptr<PostProcessor> postProcessor, int normalsMode);

        ~NormalsFallback();

        osg::ref_ptr<osg::Geometry> createGeometry(bool normals);

        osg::ref_ptr<osg::Texture> getNormalsTex() { return mNormalsTex; }

        void update(size_t frameId);

        void dirty();

        void enable();
        void disable();

    private:
        osg::ref_ptr<osg::Group> mRootNode;
        osg::ref_ptr<osg::Group> mSceneRoot;
        osg::ref_ptr<PostProcessor> mPostProcessor;
        osg::ref_ptr<osg::Texture> mNormalsTex;
        std::array<osg::ref_ptr<NormalsFallbackCamera>, 2> mCameras;
        std::array<osg::ref_ptr<CopyTextureCallback>, 2> mCopyTextureCallbacks;
        std::array<osg::ref_ptr<osg::Geometry>, 2> mNormalsFallbackGeometries;
        int mNormalsMode;
    };

}

#endif
