#ifndef STEREO_FRUSTUM_H
#define STEREO_FRUSTUM_H

#include <osg/Matrix>
#include <osg/Vec3>
#include <osg/Camera>
#include <osg/StateSet>
#include <osg/BoundingBox>

#include <memory>
#include <array>
#include <map>

#include <components/stereo/types.hpp>

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

namespace usgUtil
{
    class CullVisitor;
}

namespace SceneUtil
{
    class MWShadowTechnique;
}

namespace Stereo
{
#ifdef OSG_HAS_MULTIVIEW
    struct MultiviewFrustumCallback;
#endif

    struct ShadowFrustumCallback;

    void joinBoundingBoxes(const osg::Matrix& masterProjection, const osg::Matrix& slaveProjection, osg::BoundingBoxd& bb);
    
    class StereoFrustumManager
    {
    public:
        StereoFrustumManager(osg::Camera* camera);
        ~StereoFrustumManager();

        void update(std::array<osg::Matrix, 2> projections);

        const osg::BoundingBoxd& boundingBox() const { return mBoundingBox; }

        void setShadowTechnique(SceneUtil::MWShadowTechnique* shadowTechnique);

        void customFrustumCallback(osgUtil::CullVisitor& cv, osg::BoundingBoxd& customClipSpace, osgUtil::CullVisitor*& sharedFrustumHint);

    private:
        osg::ref_ptr<osg::Camera> mCamera;
        osg::ref_ptr<SceneUtil::MWShadowTechnique> mShadowTechnique;
        osg::ref_ptr<ShadowFrustumCallback> mShadowFrustumCallback;
        std::map< osgUtil::CullVisitor*, osgUtil::CullVisitor*> mSharedFrustums;
        osg::BoundingBoxd mBoundingBox;

#ifdef OSG_HAS_MULTIVIEW
        osg::ref_ptr<MultiviewFrustumCallback> mMultiviewFrustumCallback;
#endif
    };
}

#endif
