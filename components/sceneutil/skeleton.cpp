#include "skeleton.hpp"

#include <osg/Transform>
#include <osg/MatrixTransform>

#include <components/debug/debuglog.hpp>
#include <components/misc/stringops.hpp>

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

        mCache[Misc::StringUtils::lowerCase(bone->getName())] = std::make_pair(getNodePath(), bone);

        traverse(node);
    }
private:
    std::map<std::string, std::pair<osg::NodePath, osg::MatrixTransform*> >& mCache;
};

Skeleton::Skeleton()
    : mBoneCacheInit(false)
    , mNeedToUpdateBoneMatrices(true)
    , mActive(Active)
    , mLastFrameNumber(0)
    , mLastCullFrameNumber(0)
{

}

Skeleton::Skeleton(const Skeleton &copy, const osg::CopyOp &copyop)
    : osg::Group(copy, copyop)
    , mBoneCacheInit(false)
    , mNeedToUpdateBoneMatrices(true)
    , mActive(copy.mActive)
    , mLastFrameNumber(0)
    , mLastCullFrameNumber(0)
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

    BoneCache::iterator found = mBoneCache.find(Misc::StringUtils::lowerCase(name));
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

void Skeleton::updateBoneMatrices(unsigned int traversalNumber)
{
    if (traversalNumber != mLastFrameNumber)
        mNeedToUpdateBoneMatrices = true;

    mLastFrameNumber = traversalNumber;

    if (mNeedToUpdateBoneMatrices)
    {
        if (mRootBone.get())
        {
            for (unsigned int i=0; i<mRootBone->mChildren.size(); ++i)
                mRootBone->mChildren[i]->update(nullptr);
        }

        mNeedToUpdateBoneMatrices = false;
    }
}

void Skeleton::setActive(ActiveType active)
{
    mActive = active;
}

bool Skeleton::getActive() const
{
    return mActive != Inactive;
}

void Skeleton::markDirty()
{
    mLastFrameNumber = 0;
    mBoneCache.clear();
    mBoneCacheInit = false;
}

void Skeleton::traverse(osg::NodeVisitor& nv)
{
    if (nv.getVisitorType() == osg::NodeVisitor::UPDATE_VISITOR)
    {
        if (mActive == Inactive && mLastFrameNumber != 0)
            return;
        if (mActive == SemiActive && mLastFrameNumber != 0 && mLastCullFrameNumber+3 <= nv.getTraversalNumber())
            return;
    }
    else if (nv.getVisitorType() == osg::NodeVisitor::CULL_VISITOR)
        mLastCullFrameNumber = nv.getTraversalNumber();

    osg::Group::traverse(nv);
}

void Skeleton::childInserted(unsigned int)
{
    markDirty();
}

void Skeleton::childRemoved(unsigned int, unsigned int)
{
    markDirty();
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
        Log(Debug::Error) << "Error: Bone without node";
        return;
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
