#include "skeleton.hpp"

#include <osg/Transform>
#include <osg/MatrixTransform>

#include <iostream>

namespace SceneUtil
{

class InitBoneCacheVisitor : public osg::NodeVisitor
{
public:
    InitBoneCacheVisitor(std::map<std::string, std::pair<osg::NodePath, osg::MatrixTransform*> >& cache)
        : osg::NodeVisitor(TRAVERSE_ALL_CHILDREN)
        , mCache(cache)
    {
    }

    void apply(osg::Transform &node)
    {
        osg::MatrixTransform* bone = node.asMatrixTransform();
        if (!bone)
            return;

        mCache[bone->getName()] = std::make_pair(getNodePath(), bone);

        traverse(node);
    }
private:
    std::map<std::string, std::pair<osg::NodePath, osg::MatrixTransform*> >& mCache;
};

Skeleton::Skeleton()
    : mBoneCacheInit(false)
    , mNeedToUpdateBoneMatrices(true)
    , mActive(true)
    , mLastFrameNumber(0)
{

}

Skeleton::Skeleton(const Skeleton &copy, const osg::CopyOp &copyop)
    : osg::Group(copy, copyop)
    , mBoneCacheInit(false)
    , mNeedToUpdateBoneMatrices(true)
    , mActive(copy.mActive)
    , mLastFrameNumber(0)
{

}

Bone* Skeleton::getBone(const std::string &name)
{
    if (!mBoneCacheInit)
    {
        InitBoneCacheVisitor visitor(mBoneCache);
        accept(visitor);
        mBoneCacheInit = true;
    }

    BoneCache::iterator found = mBoneCache.find(name);
    if (found == mBoneCache.end())
        return nullptr;

    // find or insert in the bone hierarchy

    if (!mRootBone.get())
    {
        mRootBone.reset(new Bone);
    }

    const osg::NodePath& path = found->second.first;
    Bone* bone = mRootBone.get();
    for (osg::NodePath::const_iterator it = path.begin(); it != path.end(); ++it)
    {
        osg::MatrixTransform* matrixTransform = dynamic_cast<osg::MatrixTransform*>(*it);
        if (!matrixTransform)
            continue;

        Bone* child = nullptr;
        for (unsigned int i=0; i<bone->mChildren.size(); ++i)
        {
            if (bone->mChildren[i]->mNode == *it)
            {
                child = bone->mChildren[i];
                break;
            }
        }

        if (!child)
        {
            child = new Bone;
            bone->mChildren.push_back(child);
            mNeedToUpdateBoneMatrices = true;
        }
        bone = child;

        bone->mNode = matrixTransform;
    }

    return bone;
}

void Skeleton::updateBoneMatrices(osg::NodeVisitor* nv)
{
    if (nv->getFrameStamp()->getFrameNumber() != mLastFrameNumber)
        mNeedToUpdateBoneMatrices = true;

    mLastFrameNumber = nv->getFrameStamp()->getFrameNumber();

    if (mNeedToUpdateBoneMatrices)
    {
        if (mRootBone.get())
        {
            for (unsigned int i=0; i<mRootBone->mChildren.size(); ++i)
                mRootBone->mChildren[i]->update(nullptr);
        }
        else
            std::cerr << "no root bone" << std::endl;

        mNeedToUpdateBoneMatrices = false;
    }
}

void Skeleton::setActive(bool active)
{
    mActive = active;
}

bool Skeleton::getActive() const
{
    return mActive;
}

void Skeleton::traverse(osg::NodeVisitor& nv)
{
    if (!mActive && nv.getVisitorType() == osg::NodeVisitor::UPDATE_VISITOR && mLastFrameNumber != 0)
        return;
    osg::Group::traverse(nv);
}

Bone::Bone()
    : mNode(nullptr)
{
}

Bone::~Bone()
{
    for (unsigned int i=0; i<mChildren.size(); ++i)
        delete mChildren[i];
    mChildren.clear();
}

void Bone::update(const osg::Matrixf* parentMatrixInSkeletonSpace)
{
    if (!mNode)
    {
        std::cerr << "Bone without node " << std::endl;
    }
    if (parentMatrixInSkeletonSpace)
        mMatrixInSkeletonSpace = mNode->getMatrix() * (*parentMatrixInSkeletonSpace);
    else
        mMatrixInSkeletonSpace = mNode->getMatrix();

    for (unsigned int i=0; i<mChildren.size(); ++i)
    {
        mChildren[i]->update(&mMatrixInSkeletonSpace);
    }
}

}
