#include "terraindrawable.hpp"

#include <osg/ClusterCullingCallback>
#include <osgUtil/CullVisitor>

#include <components/sceneutil/lightmanager.hpp>

#include "compositemaprenderer.hpp"

namespace Terrain
{

TerrainDrawable::TerrainDrawable()
{

}

TerrainDrawable::~TerrainDrawable()
{

}

TerrainDrawable::TerrainDrawable(const TerrainDrawable &copy, const osg::CopyOp &copyop)
    : osg::Geometry(copy, copyop)
    , mPasses(copy.mPasses)
    , mLightListCallback(copy.mLightListCallback)
{

}

void TerrainDrawable::accept(osg::NodeVisitor &nv)
{
    if (nv.getVisitorType() != osg::NodeVisitor::CULL_VISITOR)
    {
        osg::Geometry::accept(nv);
    }
    else if (nv.validNodeMask(*this))
    {
        nv.pushOntoNodePath(this);
        cull(static_cast<osgUtil::CullVisitor*>(&nv));
        nv.popFromNodePath();
    }
}

inline float distance(const osg::Vec3& coord,const osg::Matrix& matrix)
{
    return -((float)coord[0]*(float)matrix(0,2)+(float)coord[1]*(float)matrix(1,2)+(float)coord[2]*(float)matrix(2,2)+matrix(3,2));
}

//canot use ClusterCullingCallback::cull: viewpoint != eyepoint
// !osgfixpotential!
bool clusterCull(osg::ClusterCullingCallback* cb, const osg::Vec3f& eyePoint, bool shadowcam)
{
    float _deviation = cb->getDeviation();
    const osg::Vec3& _controlPoint = cb->getControlPoint();
    osg::Vec3 _normal = cb->getNormal();
    if (shadowcam) _normal = _normal * -1; //inverting for shadowcam frontfaceculing
    float _radius = cb->getRadius();
    if (_deviation<=-1.0f) return false;
    osg::Vec3 eye_cp = eyePoint - _controlPoint;
    float radius = eye_cp.length();
    if (radius<_radius) return false;
    float deviation = (eye_cp * _normal)/radius;
    return deviation < _deviation;
}

void TerrainDrawable::cull(osgUtil::CullVisitor *cv)
{
    const osg::BoundingBox& bb = getBoundingBox();

    if (_cullingActive && cv->isCulled(getBoundingBox()))
        return;

    bool shadowcam = cv->getCurrentCamera()->getName() == "ShadowCamera";

    if (cv->getCullingMode() & osg::CullStack::CLUSTER_CULLING && clusterCull(mClusterCullingCallback, cv->getEyePoint(), shadowcam))
        return;

    osg::RefMatrix& matrix = *cv->getModelViewMatrix();

    if (cv->getComputeNearFarMode() && bb.valid())
    {
        if (!cv->updateCalculatedNearFar(matrix, *this, false))
            return;
    }

    float depth = bb.valid() ? distance(bb.center(),matrix) : 0.0f;
    if (osg::isNaN(depth))
        return;

    if (shadowcam)
    {
        cv->addDrawableAndDepth(this, &matrix, depth);
        return;
    }

    if (mCompositeMap)
    {
        mCompositeMapRenderer->setImmediate(mCompositeMap);
        mCompositeMap = nullptr;
    }

    bool pushedLight = mLightListCallback && mLightListCallback->pushLightState(this, cv);

    for (PassVector::const_iterator it = mPasses.begin(); it != mPasses.end(); ++it)
    {
        cv->pushStateSet(*it);
        cv->addDrawableAndDepth(this, &matrix, depth);
        cv->popStateSet();
    }

    if (pushedLight)
        cv->popStateSet();
}

void TerrainDrawable::createClusterCullingCallback()
{
    mClusterCullingCallback = new osg::ClusterCullingCallback(this);
}

void TerrainDrawable::setPasses(const TerrainDrawable::PassVector &passes)
{
    mPasses = passes;
}

void TerrainDrawable::setLightListCallback(SceneUtil::LightListCallback *lightListCallback)
{
    mLightListCallback = lightListCallback;
}

void TerrainDrawable::setupWaterBoundingBox(float waterheight, float margin)
{
    osg::Vec3Array* vertices = static_cast<osg::Vec3Array*>(getVertexArray());
    for (unsigned int i=0; i<vertices->size(); ++i)
    {
        const osg::Vec3f& vertex = (*vertices)[i];
        if (vertex.z() <= waterheight)
            mWaterBoundingBox.expandBy(vertex);
    }
    if (mWaterBoundingBox.valid())
    {
        const osg::BoundingBox& bb = getBoundingBox();
        mWaterBoundingBox.xMin() = std::max(bb.xMin(), mWaterBoundingBox.xMin() - margin);
        mWaterBoundingBox.yMin() = std::max(bb.yMin(), mWaterBoundingBox.yMin() - margin);
        mWaterBoundingBox.xMax() = std::min(bb.xMax(), mWaterBoundingBox.xMax() + margin);
        mWaterBoundingBox.xMax() = std::min(bb.xMax(), mWaterBoundingBox.xMax() + margin);
    }
}

void TerrainDrawable::compileGLObjects(osg::RenderInfo &renderInfo) const
{
    for (PassVector::const_iterator it = mPasses.begin(); it != mPasses.end(); ++it)
    {
        osg::StateSet* stateset = *it;
        stateset->compileGLObjects(*renderInfo.getState());
    }

    osg::Geometry::compileGLObjects(renderInfo);
}

}

