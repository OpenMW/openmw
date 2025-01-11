#include "terraindrawable.hpp"

#include <osg/ClusterCullingCallback>
#include <osgUtil/CullVisitor>

#include <components/sceneutil/lightmanager.hpp>

#include "compositemaprenderer.hpp"

#include "apps/openmw/mwrender/postprocessor.hpp"

namespace Terrain
{

    TerrainDrawable::TerrainDrawable() {}

    TerrainDrawable::~TerrainDrawable() {}

    TerrainDrawable::TerrainDrawable(const TerrainDrawable& copy, const osg::CopyOp& copyop)
        : osg::Geometry(copy, copyop)
        , mPasses(copy.mPasses)
        , mLightListCallback(copy.mLightListCallback)
    {
    }

    void TerrainDrawable::accept(osg::NodeVisitor& nv)
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

    inline float distance(const osg::Vec3& coord, const osg::Matrix& matrix)
    {
        return -((float)coord[0] * (float)matrix(0, 2) + (float)coord[1] * (float)matrix(1, 2)
            + (float)coord[2] * (float)matrix(2, 2) + matrix(3, 2));
    }

    // canot use ClusterCullingCallback::cull: viewpoint != eyepoint
    //  !osgfixpotential!
    bool clusterCull(osg::ClusterCullingCallback* cb, const osg::Vec3f& eyePoint, bool shadowcam)
    {
        float _deviation = cb->getDeviation();
        const osg::Vec3& _controlPoint = cb->getControlPoint();
        osg::Vec3 _normal = cb->getNormal();
        if (shadowcam)
            _normal = _normal * -1; // inverting for shadowcam frontfaceculing
        float _radius = cb->getRadius();
        if (_deviation <= -1.0f)
            return false;
        osg::Vec3 eye_cp = eyePoint - _controlPoint;
        float radius = eye_cp.length();
        if (radius < _radius)
            return false;
        float deviation = (eye_cp * _normal) / radius;
        return deviation < _deviation;
    }

    void TerrainDrawable::cull(osgUtil::CullVisitor* cv)
    {
        const osg::BoundingBox& bb = getBoundingBox();

        if (_cullingActive && cv->isCulled(getBoundingBox()))
            return;

        bool shadowcam = cv->getCurrentCamera()->getName() == "ShadowCamera";
        bool normalscam = (cv->getCurrentCamera()->getName().find("Normals Camera") == std::string::npos) ? false : true;
        bool normalMaps = false;
        bool redrawMode = false;

        if (normalscam)
        {
            normalMaps = (cv->getCurrentCamera()->getName().find("singlelayer", 14) == std::string::npos) ? false : true;
            redrawMode = (cv->getCurrentCamera()->getName().find("redraw", 14) == std::string::npos) ? false : true;

            if (redrawMode)
            {
                for (PassVector::const_iterator it = mPasses.begin(); it != mPasses.end(); ++it)
                {
                    if ((*it)->getMode(GL_BLEND) != osg::StateAttribute::ON) 
                        return;
                }
            }
        }

        if (cv->getCullingMode() & osg::CullStack::CLUSTER_CULLING
            && clusterCull(mClusterCullingCallback, cv->getEyePoint(), shadowcam))
            return;

        osg::RefMatrix& matrix = *cv->getModelViewMatrix();

        if (cv->getComputeNearFarMode() != osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR && bb.valid())
        {
            if (!cv->updateCalculatedNearFar(matrix, *this, false))
                return;
        }

        float depth = bb.valid() ? distance(bb.center(), matrix) : 0.0f;
        if (osg::isNaN(depth))
            return;

        if (shadowcam || (normalscam && !normalMaps))
        {
            cv->addDrawableAndDepth(this, &matrix, depth);
            return;
        }

        if (mCompositeMap && mCompositeMapRenderer)
        {
            mCompositeMapRenderer->setImmediate(mCompositeMap);
            mCompositeMapRenderer = nullptr;
        }

        bool pushedLight = mLightListCallback && mLightListCallback->pushLightState(this, cv);

        osg::StateSet* stateset = getStateSet();

        if (stateset && cv->getCurrentCamera()->getName() == "SceneCam")
        {
            MWRender::PostProcessor* postProcessor = static_cast<MWRender::PostProcessor*>(cv->getCurrentCamera()->getUserData());
            if (postProcessor && (postProcessor->getNormalsMode() == NormalsMode_PackedTextureFetchOnly || postProcessor->getNormalsMode() == NormalsMode_PackedTextureFetch))
            {
                stateset->setMode(GL_BLEND, (postProcessor->getNormalsEnabled()) ? osg::StateAttribute::OFF | osg::StateAttribute::OVERRIDE : osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
                stateset->setDefine("SHADER_BLENDING", (postProcessor->getNormalsEnabled()) ? "1" : "0", osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
            }
        }

        if (stateset)
            cv->pushStateSet(stateset);

        for (PassVector::const_iterator it = mPasses.begin(); it != mPasses.end(); ++it)
        {
            cv->pushStateSet(*it);
            cv->addDrawableAndDepth(this, &matrix, depth);
            cv->popStateSet();
        }

        if (stateset)
            cv->popStateSet();
        if (pushedLight)
            cv->popStateSet();
    }

    void TerrainDrawable::createClusterCullingCallback()
    {
        mClusterCullingCallback = new osg::ClusterCullingCallback(this);
    }

    void TerrainDrawable::setPasses(const TerrainDrawable::PassVector& passes)
    {
        mPasses = passes;
    }

    void TerrainDrawable::setLightListCallback(SceneUtil::LightListCallback* lightListCallback)
    {
        mLightListCallback = lightListCallback;
    }

    void TerrainDrawable::setupWaterBoundingBox(float waterheight, float margin)
    {
        osg::Vec3Array* vertices = static_cast<osg::Vec3Array*>(getVertexArray());
        for (unsigned int i = 0; i < vertices->size(); ++i)
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

    void TerrainDrawable::compileGLObjects(osg::RenderInfo& renderInfo) const
    {
        for (PassVector::const_iterator it = mPasses.begin(); it != mPasses.end(); ++it)
        {
            osg::StateSet* stateset = *it;
            stateset->compileGLObjects(*renderInfo.getState());
        }

        osg::Geometry::compileGLObjects(renderInfo);
    }

}
