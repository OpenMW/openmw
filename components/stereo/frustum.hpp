#ifndef STEREO_FRUSTUM_H
#define STEREO_FRUSTUM_H

#include <osg/BoundingBox>
#include <osg/Matrix>
#include <osg/ref_ptr>

#include <array>
#include <map>
#include <memory>

namespace osg
{
    class Camera;
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
        StereoFrustumManager(osg::Camera* camera);
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
