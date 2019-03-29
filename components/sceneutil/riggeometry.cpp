#include "riggeometry.hpp"

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
    setNumChildrenRequiringUpdateTraversal(1);
    // update done in accept(NodeVisitor&)
}

RigGeometry::RigGeometry(const RigGeometry &copy, const osg::CopyOp &copyop)
    : Drawable(copy, copyop)
    , mSkeleton(nullptr)
    , mInfluenceMap(copy.mInfluenceMap)
    , mBone2VertexVector(copy.mBone2VertexVector)
    , mBoneSphereVector(copy.mBoneSphereVector)
    , mLastFrameNumber(0)
    , mBoundsFirstFrame(true)
{
    setSourceGeometry(copy.mSourceGeometry);
    setNumChildrenRequiringUpdateTraversal(1);
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
        to.setComputeBoundingBoxCallback(new CopyBoundingBoxCallback());
        to.setComputeBoundingSphereCallback(new CopyBoundingSphereCallback());

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

    mBoneNodesVector.clear();
    for (auto& bonePair : mBoneSphereVector->mData)
    {
        const std::string& boneName = bonePair.first;
        Bone* bone = mSkeleton->getBone(boneName);
        if (!bone)
        {
            mBoneNodesVector.push_back(nullptr);
            Log(Debug::Error) << "Error: RigGeometry did not find bone " << boneName;
            continue;
        }

        mBoneNodesVector.push_back(bone);
    }

    for (auto& pair : mBone2VertexVector->mData)
    {
        for (auto &weight : pair.first)
        {
            const std::string& boneName = weight.first.first;
            Bone* bone = mSkeleton->getBone(boneName);
            if (!bone)
            {
                mBoneNodesVector.push_back(nullptr);
                Log(Debug::Error) << "Error: RigGeometry did not find bone " << boneName;
                continue;
            }

            mBoneNodesVector.push_back(bone);
        }
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

    int index = mBoneSphereVector->mData.size();
    for (auto &pair : mBone2VertexVector->mData)
    {
        osg::Matrixf resultMat (0, 0, 0, 0,
                                0, 0, 0, 0,
                                0, 0, 0, 0,
                                0, 0, 0, 1);

        for (auto &weight : pair.first)
        {
            Bone* bone = mBoneNodesVector[index];
            if (bone == nullptr)
                continue;

            accumulateMatrix(weight.first.second, bone->mMatrixInSkeletonSpace, weight.second, resultMat);
            index++;
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

    int index = 0;
    for (auto& boundPair : mBoneSphereVector->mData)
    {
        Bone* bone = mBoneNodesVector[index];
        if (bone == nullptr)
            continue;

        index++;
        osg::BoundingSpheref bs = boundPair.second;
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

        for (unsigned int i = 0; i < 2; ++i)
        {
            osg::Geometry& geom = *mGeometry[i];
            static_cast<CopyBoundingBoxCallback*>(geom.getComputeBoundingBoxCallback())->boundingBox = _boundingBox;
            static_cast<CopyBoundingSphereCallback*>(geom.getComputeBoundingSphereCallback())->boundingSphere = _boundingSphere;
            geom.dirtyBound();
        }
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

    typedef std::map<unsigned short, std::vector<BoneWeight> > Vertex2BoneMap;
    Vertex2BoneMap vertex2BoneMap;
    mBoneSphereVector = new BoneSphereVector;
    mBoneSphereVector->mData.reserve(mInfluenceMap->mData.size());
    mBone2VertexVector = new Bone2VertexVector;
    for (auto& influencePair : mInfluenceMap->mData)
    {
        const std::string& boneName = influencePair.first;
        const BoneInfluence& bi = influencePair.second;
        mBoneSphereVector->mData.emplace_back(boneName, bi.mBoundSphere);

        for (auto& weightPair: bi.mWeights)
        {
            std::vector<BoneWeight>& vec = vertex2BoneMap[weightPair.first];

            vec.emplace_back(std::make_pair(boneName, bi.mInvBindMatrix), weightPair.second);
        }
    }

    Bone2VertexMap bone2VertexMap;
    for (auto& vertexPair : vertex2BoneMap)
    {
        bone2VertexMap[vertexPair.second].emplace_back(vertexPair.first);
    }

    mBone2VertexVector->mData.reserve(bone2VertexMap.size());
    mBone2VertexVector->mData.assign(bone2VertexMap.begin(), bone2VertexMap.end());
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
