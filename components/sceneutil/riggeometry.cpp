#include "riggeometry.hpp"

#include <stdexcept>
#include <iostream>
#include <cstdlib>

#include <osg/Version>
#include <osg/MatrixTransform>

#include "skeleton.hpp"
#include "util.hpp"

namespace SceneUtil
{

class UpdateRigBounds : public osg::Drawable::UpdateCallback
{
public:
    UpdateRigBounds()
    {
    }

    UpdateRigBounds(const UpdateRigBounds& copy, const osg::CopyOp& copyop)
        : osg::Drawable::UpdateCallback(copy, copyop)
    {
    }

    META_Object(SceneUtil, UpdateRigBounds)

    void update(osg::NodeVisitor* nv, osg::Drawable* drw)
    {
        RigGeometry* rig = static_cast<RigGeometry*>(drw);

        rig->updateBounds(nv);
    }
};

// TODO: make threadsafe for multiple cull threads
class UpdateRigGeometry : public osg::Drawable::CullCallback
{
public:
    UpdateRigGeometry()
    {
    }

    UpdateRigGeometry(const UpdateRigGeometry& copy, const osg::CopyOp& copyop)
        : osg::Drawable::CullCallback(copy, copyop)
    {
    }

    META_Object(SceneUtil, UpdateRigGeometry)

    virtual bool cull(osg::NodeVisitor* nv, osg::Drawable* drw, osg::State*) const
    {
        RigGeometry* geom = static_cast<RigGeometry*>(drw);
        geom->update(nv);
        return false;
    }
};

// We can't compute the bounds without a NodeVisitor, since we need the current geomToSkelMatrix.
// So we return nothing. Bounds are updated every frame in the UpdateCallback.
class DummyComputeBoundCallback : public osg::Drawable::ComputeBoundingBoxCallback
{
public:
    virtual osg::BoundingBox computeBound(const osg::Drawable&) const  { return osg::BoundingBox(); }
};

RigGeometry::RigGeometry()
    : mSkeleton(NULL)
    , mLastFrameNumber(0)
    , mBoundsFirstFrame(true)
{
    setCullCallback(new UpdateRigGeometry);
    setUpdateCallback(new UpdateRigBounds);
    setSupportsDisplayList(false);
    setComputeBoundingBoxCallback(new DummyComputeBoundCallback);
}

RigGeometry::RigGeometry(const RigGeometry &copy, const osg::CopyOp &copyop)
    : osg::Geometry(copy, copyop)
    , mSkeleton(NULL)
    , mInfluenceMap(copy.mInfluenceMap)
    , mLastFrameNumber(0)
    , mBoundsFirstFrame(copy.mBoundsFirstFrame)
{
    setSourceGeometry(copy.mSourceGeometry);
}

void RigGeometry::setSourceGeometry(osg::ref_ptr<osg::Geometry> sourceGeometry)
{
    mSourceGeometry = sourceGeometry;

    osg::Geometry& from = *sourceGeometry;

    if (from.getStateSet())
        setStateSet(from.getStateSet());

    // copy over primitive sets.
    getPrimitiveSetList() = from.getPrimitiveSetList();

    if (from.getColorArray())
        setColorArray(from.getColorArray());

    if (from.getSecondaryColorArray())
        setSecondaryColorArray(from.getSecondaryColorArray());

    if (from.getFogCoordArray())
        setFogCoordArray(from.getFogCoordArray());

    for(unsigned int ti=0;ti<from.getNumTexCoordArrays();++ti)
    {
        if (from.getTexCoordArray(ti))
            setTexCoordArray(ti,from.getTexCoordArray(ti));
    }

    osg::Geometry::ArrayList& arrayList = from.getVertexAttribArrayList();
    for(unsigned int vi=0;vi< arrayList.size();++vi)
    {
        osg::Array* array = arrayList[vi].get();
        if (array)
            setVertexAttribArray(vi,array);
    }

    setVertexArray(dynamic_cast<osg::Array*>(from.getVertexArray()->clone(osg::CopyOp::DEEP_COPY_ALL)));
    setNormalArray(dynamic_cast<osg::Array*>(from.getNormalArray()->clone(osg::CopyOp::DEEP_COPY_ALL)), osg::Array::BIND_PER_VERTEX);
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
        std::cerr << "A RigGeometry did not find its parent skeleton" << std::endl;
        return false;
    }

    if (!mInfluenceMap)
    {
        std::cerr << "No InfluenceMap set on RigGeometry" << std::endl;
        return false;
    }

    typedef std::map<unsigned short, std::vector<BoneWeight> > Vertex2BoneMap;
    Vertex2BoneMap vertex2BoneMap;
    for (std::map<std::string, BoneInfluence>::const_iterator it = mInfluenceMap->mMap.begin(); it != mInfluenceMap->mMap.end(); ++it)
    {
        Bone* bone = mSkeleton->getBone(it->first);
        if (!bone)
        {
            std::cerr << "RigGeometry did not find bone " << it->first << std::endl;
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

    for (Vertex2BoneMap::iterator it = vertex2BoneMap.begin(); it != vertex2BoneMap.end(); it++)
    {
        mBone2VertexMap[it->second].push_back(it->first);
    }

    return true;
}

void accummulateMatrix(const osg::Matrixf& invBindMatrix, const osg::Matrixf& matrix, float weight, osg::Matrixf& result)
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

void RigGeometry::update(osg::NodeVisitor* nv)
{
    if (!mSkeleton)
    {
        if (!initFromParentSkeleton(nv))
            return;
    }

    if (!mSkeleton->getActive() && mLastFrameNumber != 0)
        return;

    if (mLastFrameNumber == nv->getFrameStamp()->getFrameNumber())
        return;
    mLastFrameNumber = nv->getFrameStamp()->getFrameNumber();

    mSkeleton->updateBoneMatrices(nv);

    osg::Matrixf geomToSkel = getGeomToSkelMatrix(nv);

    // skinning
    osg::Vec3Array* positionSrc = static_cast<osg::Vec3Array*>(mSourceGeometry->getVertexArray());
    osg::Vec3Array* normalSrc = static_cast<osg::Vec3Array*>(mSourceGeometry->getNormalArray());

    osg::Vec3Array* positionDst = static_cast<osg::Vec3Array*>(getVertexArray());
    osg::Vec3Array* normalDst = static_cast<osg::Vec3Array*>(getNormalArray());

    for (Bone2VertexMap::const_iterator it = mBone2VertexMap.begin(); it != mBone2VertexMap.end(); ++it)
    {
        osg::Matrixf resultMat  (0, 0, 0, 0,
                                0, 0, 0, 0,
                                0, 0, 0, 0,
                                0, 0, 0, 1);

        for (std::vector<BoneWeight>::const_iterator weightIt = it->first.begin(); weightIt != it->first.end(); ++weightIt)
        {
            Bone* bone = weightIt->first.first;
            const osg::Matrix& invBindMatrix = weightIt->first.second;
            float weight = weightIt->second;
            const osg::Matrixf& boneMatrix = bone->mMatrixInSkeletonSpace;
            accummulateMatrix(invBindMatrix, boneMatrix, weight, resultMat);
        }
        resultMat = resultMat * geomToSkel;

        for (std::vector<unsigned short>::const_iterator vertexIt = it->second.begin(); vertexIt != it->second.end(); ++vertexIt)
        {
            unsigned short vertex = *vertexIt;
            (*positionDst)[vertex] = resultMat.preMult((*positionSrc)[vertex]);
            (*normalDst)[vertex] = osg::Matrix::transform3x3((*normalSrc)[vertex], resultMat);
        }
    }

    positionDst->dirty();
    normalDst->dirty();
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

    mSkeleton->updateBoneMatrices(nv);

    osg::Matrixf geomToSkel = getGeomToSkelMatrix(nv);
    osg::BoundingBox box;
    for (BoneSphereMap::const_iterator it = mBoneSphereMap.begin(); it != mBoneSphereMap.end(); ++it)
    {
        Bone* bone = it->first;
        osg::BoundingSpheref bs = it->second;
        transformBoundingSphere(bone->mMatrixInSkeletonSpace * geomToSkel, bs);
        box.expandBy(bs);
    }

    _boundingBox = box;
    _boundingBoxComputed = true;
#if OSG_VERSION_GREATER_OR_EQUAL(3,3,3)
    // in OSG 3.3.3 and up Drawable inherits from Node, so has a bounding sphere as well.
    _boundingSphere = osg::BoundingSphere(_boundingBox);
    _boundingSphereComputed = true;
#endif
    for (unsigned int i=0; i<getNumParents(); ++i)
        getParent(i)->dirtyBound();
}

osg::Matrixf RigGeometry::getGeomToSkelMatrix(osg::NodeVisitor *nv)
{
    osg::NodePath path;
    bool foundSkel = false;
    for (osg::NodePath::const_iterator it = nv->getNodePath().begin(); it != nv->getNodePath().end(); ++it)
    {
        if (!foundSkel)
        {
            if (*it == mSkeleton)
                foundSkel = true;
        }
        else
            path.push_back(*it);
    }
    return osg::computeWorldToLocal(path);

}

void RigGeometry::setInfluenceMap(osg::ref_ptr<InfluenceMap> influenceMap)
{
    mInfluenceMap = influenceMap;
}


}
