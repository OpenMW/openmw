#ifndef OPENMW_COMPONENTS_NIFOSG_SKELETON_H
#define OPENMW_COMPONENTS_NIFOSG_SKELETON_H

#include <osg/Group>

#include <memory>

namespace NifOsg
{

    // Defines a Bone hierarchy, used for updating of skeleton-space bone matrices.
    // To prevent unnecessary updates, only bones that are used for skinning will be added to this hierarchy.
    class Bone
    {
    public:
        Bone();
        ~Bone();

        osg::Matrix mMatrixInSkeletonSpace;

        osg::MatrixTransform* mNode;

        std::vector<Bone*> mChildren;

        void update(const osg::Matrixf* parentMatrixInSkeletonSpace);

    private:
        Bone(const Bone&);
        void operator=(const Bone&);
    };

    class Skeleton : public osg::Group
    {
    public:
        Skeleton();
        Skeleton(const Skeleton& copy, const osg::CopyOp& copyop);

        Bone* getBone(const std::string& name);

        META_Node(NifOsg, Skeleton)

        void updateBoneMatrices();

    private:
        // The root bone is not a "real" bone, it has no corresponding node in the scene graph.
        // As far as the scene graph goes we support multiple root bones.
        std::auto_ptr<Bone> mRootBone;

        typedef std::map<std::string, std::pair<osg::NodePath, osg::MatrixTransform*> > BoneCache;
        BoneCache mBoneCache;
        bool mBoneCacheInit;

        bool mNeedToUpdateBoneMatrices;
    };

}

#endif
