#include "riggeometry.hpp"

#include <stdexcept>
#include <cstdlib>

#include <osg/Version>

#include <components/debug/debuglog.hpp>

#include "skeleton.hpp"
#include "util.hpp"

namespace
{
    inline void accumulateMatrix(const osg::Matrixf& invBindMatrix, const osg::Matrixf& matrix, const float weight, osg::Matrixf& result)
    {
        osg::Matrixf m = invBindMatrix * matrix;
        float* ptr = m.ptr();
        float* ptrresult = result.ptr();
        ptrresult[0] += ptr[0] * weight;
        ptrresult[1] += ptr[1] * weight;
        ptrresult[2] += ptr[2] * weight;

        ptrresult[4] += ptr[4] * weight;
        ptrresult[5] += ptr[5] * weight;
        ptrresult[6] += ptr[6] * weight;

        ptrresult[8] += ptr[8] * weight;
        ptrresult[9] += ptr[9] * weight;
        ptrresult[10] += ptr[10] * weight;

        ptrresult[12] += ptr[12] * weight;
        ptrresult[13] += ptr[13] * weight;
        ptrresult[14] += ptr[14] * weight;
    }
}

namespace SceneUtil
{

RigGeometry::RigGeometry()
    : mSkeleton(nullptr)
    , mLastFrameNumber(0)
    , mBoundsFirstFrame(true)
{
    setUpdateCallback(new osg::Callback); // dummy to make sure getNumChildrenRequiringUpdateTraversal() is correct
                                          // update done in accept(NodeVisitor&)
}

RigGeometry::RigGeometry(const RigGeometry &copy, const osg::CopyOp &copyop)
    : Drawable(copy, copyop)
    , mSkeleton(nullptr)
    , mInfluenceMap(copy.mInfluenceMap)
    , mLastFrameNumber(0)
    , mBoundsFirstFrame(true)
{
    setSourceGeometry(copy.mSourceGeometry);
}

void RigGeometry::setSourceGeometry(osg::ref_ptr<osg::Geometry> sourceGeometry)
{
    mSourceGeometry = sourceGeometry;

    for (unsigned int i=0; i<2; ++i)
    {
        const osg::Geometry& from = *sourceGeometry;
        mGeometry[i] = new osg::Geometry(from, osg::CopyOp::SHALLOW_COPY);
        osg::Geometry& to = *mGeometry[i];
        to.setSupportsDisplayList(false);
        to.setUseVertexBufferObjects(true);
        to.setCullingActive(false); // make sure to disable culling since that's handled by this class

        // vertices and normals are modified every frame, so we need to deep copy them.
        // assign a dedicated VBO to make sure that modifications don't interfere with source geometry's VBO.
        osg::ref_ptr<osg::VertexBufferObject> vbo (new osg::VertexBufferObject);
        vbo->setUsage(GL_DYNAMIC_DRAW_ARB);

        osg::ref_ptr<osg::Array> vertexArray = osg::clone(from.getVertexArray(), osg::CopyOp::DEEP_COPY_ALL);
        if (vertexArray)
        {
            vertexArray->setVertexBufferObject(vbo);
            to.setVertexArray(vertexArray);
        }

        if (const osg::Array* normals = from.getNormalArray())
        {
            osg::ref_ptr<osg::Array> normalArray = osg::clone(normals, osg::CopyOp::DEEP_COPY_ALL);
            if (normalArray)
            {
                normalArray->setVertexBufferObject(vbo);
                to.setNormalArray(normalArray, osg::Array::BIND_PER_VERTEX);
            }
        }

        if (const osg::Vec4Array* tangents = dynamic_cast<const osg::Vec4Array*>(from.getTexCoordArray(7)))
        {
            mSourceTangents = tangents;
            osg::ref_ptr<osg::Array> tangentArray = osg::clone(tangents, osg::CopyOp::DEEP_COPY_ALL);
            tangentArray->setVertexBufferObject(vbo);
            to.setTexCoordArray(7, tangentArray, osg::Array::BIND_PER_VERTEX);
        }
        else
            mSourceTangents = nullptr;
    }
}

osg::ref_ptr<osg::Geometry> RigGeometry::getSourceGeometry()
{
    return mSourceGeometry;
}

bool RigGeometry::initFromParentSkeleton(osg::NodeVisitor* nv)
{
    const osg::NodePath& path = nv->getNodePath();
    for (osg::NodePath::const_reverse_iterator it = path.rbegin(); it != path.rend(); ++it)
    {
        osg::Node* node = *it;
        if (Skeleton* skel = dynamic_cast<Skeleton*>(node))
        {
            mSkeleton = skel;
            break;
        }
    }

    if (!mSkeleton)
    {
        Log(Debug::Error) << "Error: A RigGeometry did not find its parent skeleton";
        return false;
    }

    if (!mInfluenceMap)
    {
        Log(Debug::Error) << "Error: No InfluenceMap set on RigGeometry";
        return false;
    }

    typedef std::map<unsigned short, std::vector<BoneWeight> > Vertex2BoneMap;
    Vertex2BoneMap vertex2BoneMap;
    for (std::map<std::string, BoneInfluence>::const_iterator it = mInfluenceMap->mMap.begin(); it != mInfluenceMap->mMap.end(); ++it)
    {
        Bone* bone = mSkeleton->getBone(it->first);
        if (!bone)
        {
            Log(Debug::Error) << "Error: RigGeometry did not find bone " << it->first ;
            continue;
        }

        mBoneSphereMap[bone] = it->second.mBoundSphere;

        const BoneInfluence& bi = it->second;

        const std::map<unsigned short, float>& weights = it->second.mWeights;
        for (std::map<unsigned short, float>::const_iterator weightIt = weights.begin(); weightIt != weights.end(); ++weightIt)
        {
            std::vector<BoneWeight>& vec = vertex2BoneMap[weightIt->first];

            BoneWeight b = std::make_pair(std::make_pair(bone, bi.mInvBindMatrix), weightIt->second);

            vec.push_back(b);
        }
    }

    for (Vertex2BoneMap::iterator it = vertex2BoneMap.begin(); it != vertex2BoneMap.end(); ++it)
    {
        mBone2VertexMap[it->second].push_back(it->first);
    }

    return true;
}

void RigGeometry::cull(osg::NodeVisitor* nv)
{
    if (!mSkeleton)
    {
        Log(Debug::Error) << "Error: RigGeometry rendering with no skeleton, should have been initialized by UpdateVisitor";
        // try to recover anyway, though rendering is likely to be incorrect.
        if (!initFromParentSkeleton(nv))
            return;
    }

    unsigned int traversalNumber = nv->getTraversalNumber();
    if (mLastFrameNumber == traversalNumber || (mLastFrameNumber != 0 && !mSkeleton->getActive()))
    {
        osg::Geometry& geom = *getGeometry(mLastFrameNumber);
        nv->pushOntoNodePath(&geom);
        nv->apply(geom);
        nv->popFromNodePath();
        return;
    }
    mLastFrameNumber = traversalNumber;
    osg::Geometry& geom = *getGeometry(mLastFrameNumber);

    mSkeleton->updateBoneMatrices(traversalNumber);

    // skinning
    const osg::Vec3Array* positionSrc = static_cast<osg::Vec3Array*>(mSourceGeometry->getVertexArray());
    const osg::Vec3Array* normalSrc = static_cast<osg::Vec3Array*>(mSourceGeometry->getNormalArray());
    const osg::Vec4Array* tangentSrc = mSourceTangents;

    osg::Vec3Array* positionDst = static_cast<osg::Vec3Array*>(geom.getVertexArray());
    osg::Vec3Array* normalDst = static_cast<osg::Vec3Array*>(geom.getNormalArray());
    osg::Vec4Array* tangentDst = static_cast<osg::Vec4Array*>(geom.getTexCoordArray(7));

    for (auto &pair : mBone2VertexMap)
    {
        osg::Matrixf resultMat (0, 0, 0, 0,
                                0, 0, 0, 0,
                                0, 0, 0, 0,
                                0, 0, 0, 1);

        for (auto &weight : pair.first)
        {
            accumulateMatrix(weight.first.second, weight.first.first->mMatrixInSkeletonSpace, weight.second, resultMat);
        }

        if (mGeomToSkelMatrix)
            resultMat *= (*mGeomToSkelMatrix);

        for (auto &vertex : pair.second)
        {
            (*positionDst)[vertex] = resultMat.preMult((*positionSrc)[vertex]);
            if (normalDst)
                (*normalDst)[vertex] = osg::Matrixf::transform3x3((*normalSrc)[vertex], resultMat);

            if (tangentDst)
            {
                const osg::Vec4f& srcTangent = (*tangentSrc)[vertex];
                osg::Vec3f transformedTangent = osg::Matrixf::transform3x3(osg::Vec3f(srcTangent.x(), srcTangent.y(), srcTangent.z()), resultMat);
                (*tangentDst)[vertex] = osg::Vec4f(transformedTangent, srcTangent.w());
            }
        }
    }

    positionDst->dirty();
    if (normalDst)
        normalDst->dirty();
    if (tangentDst)
        tangentDst->dirty();

#if OSG_MIN_VERSION_REQUIRED(3, 5, 6)
    geom.dirtyGLObjects();
#endif

    nv->pushOntoNodePath(&geom);
    nv->apply(geom);
    nv->popFromNodePath();
}

void RigGeometry::updateBounds(osg::NodeVisitor *nv)
{
    if (!mSkeleton)
    {
        if (!initFromParentSkeleton(nv))
            return;
    }

    if (!mSkeleton->getActive() && !mBoundsFirstFrame)
        return;
    mBoundsFirstFrame = false;

    mSkeleton->updateBoneMatrices(nv->getTraversalNumber());

    updateGeomToSkelMatrix(nv->getNodePath());

    osg::BoundingBox box;
    for (BoneSphereMap::const_iterator it = mBoneSphereMap.begin(); it != mBoneSphereMap.end(); ++it)
    {
        Bone* bone = it->first;
        osg::BoundingSpheref bs = it->second;
        if (mGeomToSkelMatrix)
            transformBoundingSphere(bone->mMatrixInSkeletonSpace * (*mGeomToSkelMatrix), bs);
        else
            transformBoundingSphere(bone->mMatrixInSkeletonSpace, bs);
        box.expandBy(bs);
    }

    if (box != _boundingBox)
    {
        _boundingBox = box;
        _boundingSphere = osg::BoundingSphere(_boundingBox);
        _boundingSphereComputed = true;
        for (unsigned int i=0; i<getNumParents(); ++i)
            getParent(i)->dirtyBound();
    }
}

void RigGeometry::updateGeomToSkelMatrix(const osg::NodePath& nodePath)
{
    bool foundSkel = false;
    osg::ref_ptr<osg::RefMatrix> geomToSkelMatrix;
    for (osg::NodePath::const_iterator it = nodePath.begin(); it != nodePath.end(); ++it)
    {
        osg::Node* node = *it;
        if (!foundSkel)
        {
            if (node == mSkeleton)
                foundSkel = true;
        }
        else
        {
            if (osg::Transform* trans = node->asTransform())
            {
                if (!geomToSkelMatrix)
                    geomToSkelMatrix = new osg::RefMatrix;
                trans->computeWorldToLocalMatrix(*geomToSkelMatrix, nullptr);
            }
        }
    }
    if (geomToSkelMatrix && !geomToSkelMatrix->isIdentity())
        mGeomToSkelMatrix = geomToSkelMatrix;
}

void RigGeometry::setInfluenceMap(osg::ref_ptr<InfluenceMap> influenceMap)
{
    mInfluenceMap = influenceMap;
}

void RigGeometry::accept(osg::NodeVisitor &nv)
{
    if (!nv.validNodeMask(*this))
        return;

    nv.pushOntoNodePath(this);

    if (nv.getVisitorType() == osg::NodeVisitor::CULL_VISITOR)
        cull(&nv);
    else if (nv.getVisitorType() == osg::NodeVisitor::UPDATE_VISITOR)
        updateBounds(&nv);
    else
        nv.apply(*this);

    nv.popFromNodePath();
}

void RigGeometry::accept(osg::PrimitiveFunctor& func) const
{
    getGeometry(mLastFrameNumber)->accept(func);
}

osg::Geometry* RigGeometry::getGeometry(unsigned int frame) const
{
    return mGeometry[frame%2].get();
}


}
