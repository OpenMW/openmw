#include "riggeometry.hpp"

#include <unordered_map>

#include <osg/MatrixTransform>

#include <osgUtil/CullVisitor>

#include <components/debug/debuglog.hpp>
#include <components/resource/scenemanager.hpp>

#include "skeleton.hpp"
#include "util.hpp"

namespace SceneUtil
{

    RigGeometry::RigGeometry()
    {
        setNumChildrenRequiringUpdateTraversal(1);
        // update done in accept(NodeVisitor&)
    }

    RigGeometry::RigGeometry(const RigGeometry& copy, const osg::CopyOp& copyop)
        : Drawable(copy, copyop)
        , mData(copy.mData)
    {
        setSourceGeometry(copy.mSourceGeometry);
        setNumChildrenRequiringUpdateTraversal(1);
    }

    void RigGeometry::setSourceGeometry(osg::ref_ptr<osg::Geometry> sourceGeometry)
    {
        for (unsigned int i = 0; i < 2; ++i)
            mGeometry[i] = nullptr;

        mSourceGeometry = sourceGeometry;

        for (unsigned int i = 0; i < 2; ++i)
        {
            const osg::Geometry& from = *sourceGeometry;

            // DO NOT COPY AND PASTE THIS CODE. Cloning osg::Geometry without also cloning its contained Arrays is
            // generally unsafe. In this specific case the operation is safe under the following two assumptions:
            // - When Arrays are removed or replaced in the cloned geometry, the original Arrays in their place must
            // outlive the cloned geometry regardless. (ensured by mSourceGeometry)
            // - Arrays that we add or replace in the cloned geometry must be explicitely forbidden from reusing
            // BufferObjects of the original geometry. (ensured by vbo below)
            mGeometry[i] = new osg::Geometry(from, osg::CopyOp::SHALLOW_COPY);
            mGeometry[i]->getOrCreateUserDataContainer()->addUserObject(new Resource::TemplateRef(mSourceGeometry));

            osg::Geometry& to = *mGeometry[i];
            to.setSupportsDisplayList(false);
            to.setUseVertexBufferObjects(true);
            to.setCullingActive(false); // make sure to disable culling since that's handled by this class
            to.setComputeBoundingBoxCallback(new CopyBoundingBoxCallback());
            to.setComputeBoundingSphereCallback(new CopyBoundingSphereCallback());

            // vertices and normals are modified every frame, so we need to deep copy them.
            // assign a dedicated VBO to make sure that modifications don't interfere with source geometry's VBO.
            osg::ref_ptr<osg::VertexBufferObject> vbo(new osg::VertexBufferObject);
            vbo->setUsage(GL_DYNAMIC_DRAW_ARB);

            osg::ref_ptr<osg::Array> vertexArray
                = static_cast<osg::Array*>(from.getVertexArray()->clone(osg::CopyOp::DEEP_COPY_ALL));
            if (vertexArray)
            {
                vertexArray->setVertexBufferObject(vbo);
                to.setVertexArray(vertexArray);
            }

            if (const osg::Array* normals = from.getNormalArray())
            {
                osg::ref_ptr<osg::Array> normalArray
                    = static_cast<osg::Array*>(normals->clone(osg::CopyOp::DEEP_COPY_ALL));
                if (normalArray)
                {
                    normalArray->setVertexBufferObject(vbo);
                    to.setNormalArray(normalArray, osg::Array::BIND_PER_VERTEX);
                }
            }

            if (const osg::Vec4Array* tangents = dynamic_cast<const osg::Vec4Array*>(from.getTexCoordArray(7)))
            {
                mSourceTangents = tangents;
                osg::ref_ptr<osg::Array> tangentArray
                    = static_cast<osg::Array*>(tangents->clone(osg::CopyOp::DEEP_COPY_ALL));
                tangentArray->setVertexBufferObject(vbo);
                to.setTexCoordArray(7, tangentArray, osg::Array::BIND_PER_VERTEX);
            }
            else
                mSourceTangents = nullptr;
        }
    }

    osg::ref_ptr<osg::Geometry> RigGeometry::getSourceGeometry() const
    {
        return mSourceGeometry;
    }

    bool RigGeometry::initFromParentSkeleton(osg::NodeVisitor* nv)
    {
        const osg::NodePath& path = nv->getNodePath();
        for (osg::NodePath::const_reverse_iterator it = path.rbegin() + 1; it != path.rend(); ++it)
        {
            osg::Node* node = *it;
            if (node->asTransform())
                continue;
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

        if (!mData)
        {
            Log(Debug::Error) << "Error: No influence data set on RigGeometry";
            return false;
        }

        mNodes.clear();
        for (const BoneInfo& info : mData->mBones)
        {
            mNodes.push_back(mSkeleton->getBone(info.mName));
            if (!mNodes.back())
                Log(Debug::Error) << "Error: RigGeometry did not find bone " << info.mName;
        }

        return true;
    }

    void RigGeometry::cull(osg::NodeVisitor* nv)
    {
        if (!mSkeleton)
        {
            Log(Debug::Error)
                << "Error: RigGeometry rendering with no skeleton, should have been initialized by UpdateVisitor";
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

        std::vector<osg::Matrixf> boneMatrices(mNodes.size());
        std::vector<Bone*>::const_iterator bone = mNodes.begin();
        std::vector<BoneInfo>::const_iterator boneInfo = mData->mBones.begin();
        for (osg::Matrixf& boneMat : boneMatrices)
        {
            if (*bone != nullptr)
                boneMat = boneInfo->mInvBindMatrix * (*bone)->mMatrixInSkeletonSpace;
            ++bone;
            ++boneInfo;
        }

        for (const auto& [influences, vertices] : mData->mInfluences)
        {
            osg::Matrixf resultMat(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1);

            for (const auto& [index, weight] : influences)
            {
                if (mNodes[index] == nullptr)
                    continue;
                const float* boneMatPtr = boneMatrices[index].ptr();
                float* resultMatPtr = resultMat.ptr();
                for (int i = 0; i < 16; ++i, ++resultMatPtr, ++boneMatPtr)
                    if (i % 4 != 3)
                        *resultMatPtr += *boneMatPtr * weight;
            }

            if (mGeomToSkelMatrix)
                resultMat *= (*mGeomToSkelMatrix);

            for (unsigned short vertex : vertices)
            {
                (*positionDst)[vertex] = resultMat.preMult((*positionSrc)[vertex]);
                if (normalDst)
                    (*normalDst)[vertex] = osg::Matrixf::transform3x3((*normalSrc)[vertex], resultMat);

                if (tangentDst)
                {
                    const osg::Vec4f& srcTangent = (*tangentSrc)[vertex];
                    osg::Vec3f transformedTangent = osg::Matrixf::transform3x3(
                        osg::Vec3f(srcTangent.x(), srcTangent.y(), srcTangent.z()), resultMat);
                    (*tangentDst)[vertex] = osg::Vec4f(transformedTangent, srcTangent.w());
                }
            }
        }

        positionDst->dirty();
        if (normalDst)
            normalDst->dirty();
        if (tangentDst)
            tangentDst->dirty();

        geom.osg::Drawable::dirtyGLObjects();

        nv->pushOntoNodePath(&geom);
        nv->apply(geom);
        nv->popFromNodePath();
    }

    void RigGeometry::updateBounds(osg::NodeVisitor* nv)
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

        size_t index = 0;
        for (const BoneInfo& info : mData->mBones)
        {
            const Bone* bone = mNodes[index++];
            if (bone == nullptr)
                continue;

            osg::BoundingSpheref bs = info.mBoundSphere;
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
            for (unsigned int i = 0; i < getNumParents(); ++i)
                getParent(i)->dirtyBound();

            for (unsigned int i = 0; i < 2; ++i)
            {
                osg::Geometry& geom = *mGeometry[i];
                static_cast<CopyBoundingBoxCallback*>(geom.getComputeBoundingBoxCallback())->boundingBox = _boundingBox;
                static_cast<CopyBoundingSphereCallback*>(geom.getComputeBoundingSphereCallback())->boundingSphere
                    = _boundingSphere;
                geom.dirtyBound();
            }
        }
    }

    void RigGeometry::updateGeomToSkelMatrix(const osg::NodePath& nodePath)
    {
        bool foundSkel = false;
        osg::RefMatrix* geomToSkelMatrix = mGeomToSkelMatrix;
        if (geomToSkelMatrix)
            geomToSkelMatrix->makeIdentity();
        for (osg::NodePath::const_iterator it = nodePath.begin(); it != nodePath.end() - 1; ++it)
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
                    osg::MatrixTransform* matrixTrans = trans->asMatrixTransform();
                    if (matrixTrans && matrixTrans->getMatrix().isIdentity())
                        continue;
                    if (!geomToSkelMatrix)
                        geomToSkelMatrix = mGeomToSkelMatrix = new osg::RefMatrix;
                    trans->computeWorldToLocalMatrix(*geomToSkelMatrix, nullptr);
                }
            }
        }
    }

    void RigGeometry::setBoneInfo(std::vector<BoneInfo>&& bones)
    {
        if (!mData)
            mData = new InfluenceData;

        mData->mBones = std::move(bones);
    }

    void RigGeometry::setInfluences(const std::vector<VertexWeights>& influences)
    {
        if (!mData)
            mData = new InfluenceData;

        std::unordered_map<unsigned short, BoneWeights> vertexToInfluences;
        size_t index = 0;
        for (const auto& influence : influences)
        {
            for (const auto& [vertex, weight] : influence)
                vertexToInfluences[vertex].emplace_back(index, weight);
            index++;
        }

        std::map<BoneWeights, VertexList> influencesToVertices;
        for (const auto& [vertex, weights] : vertexToInfluences)
            influencesToVertices[weights].emplace_back(vertex);

        mData->mInfluences.reserve(influencesToVertices.size());
        mData->mInfluences.assign(influencesToVertices.begin(), influencesToVertices.end());
    }

    void RigGeometry::setInfluences(const std::vector<BoneWeights>& influences)
    {
        if (!mData)
            mData = new InfluenceData;

        std::map<BoneWeights, VertexList> influencesToVertices;
        for (size_t i = 0; i < influences.size(); i++)
            influencesToVertices[influences[i]].emplace_back(i);

        mData->mInfluences.reserve(influencesToVertices.size());
        mData->mInfluences.assign(influencesToVertices.begin(), influencesToVertices.end());
    }

    void RigGeometry::accept(osg::NodeVisitor& nv)
    {
        if (!nv.validNodeMask(*this))
            return;

        nv.pushOntoNodePath(this);

        if (nv.getVisitorType() == osg::NodeVisitor::CULL_VISITOR)
        {
            // The cull visitor won't be applied to the node itself,
            // but we want to use its state to render the child geometry.
            osg::StateSet* stateset = getStateSet();
            osgUtil::CullVisitor* cv = static_cast<osgUtil::CullVisitor*>(&nv);
            if (stateset)
                cv->pushStateSet(stateset);

            cull(&nv);
            if (stateset)
                cv->popStateSet();
        }
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
        return mGeometry[frame % 2].get();
    }

}
