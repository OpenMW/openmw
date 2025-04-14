#ifndef STEREO_FRUSTUM_H
#define STEREO_FRUSTUM_H

#include <osg/BoundingBox>
#include <osg/Camera>
#include <osg/Matrix>
#include <osg/StateSet>
#include <osg/Vec3>

#include <array>
#include <map>
#include <memory>

namespace osg
{
    class FrameBufferObject;
    class Texture2D;
    class Texture2DMultisample;
    class Texture2DArray;
}

namespace osgViewer
{
    class Viewer;
}

namespace osgUtil
{
    class CullVisitor;
}

namespace SceneUtil
{
    class MWShadowTechnique;
}

namespace Stereo
{
    struct MultiviewFrustumCallback;
    struct ShadowFrustumCallback;

    void joinBoundingBoxes(
        const osg::Matrix& masterProjection, const osg::Matrix& slaveProjection, osg::BoundingBoxd& bb);

    class StereoFrustumManager
    {
    public:
        StereoFrustumManager(bool sharedShadowMaps, osg::Camera* camera);
        ~StereoFrustumManager();

        void update(std::array<osg::Matrix, 2> projections);

        const osg::BoundingBoxd& boundingBox() const { return mBoundingBox; }

        void setShadowTechnique(SceneUtil::MWShadowTechnique* shadowTechnique);

        void customFrustumCallback(
            osgUtil::CullVisitor& cv, osg::BoundingBoxd& customClipSpace, osgUtil::CullVisitor*& sharedFrustumHint);

    private:
        osg::ref_ptr<osg::Camera> mCamera;
        osg::ref_ptr<SceneUtil::MWShadowTechnique> mShadowTechnique;
        osg::ref_ptr<ShadowFrustumCallback> mShadowFrustumCallback;
        std::map<osgUtil::CullVisitor*, osgUtil::CullVisitor*> mSharedFrustums;
        osg::BoundingBoxd mBoundingBox;

        std::unique_ptr<MultiviewFrustumCallback> mMultiviewFrustumCallback;
    };
}

#endif
