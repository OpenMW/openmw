#ifndef OPENMW_COMPONENTS_NIFOSG_SKELETON_H
#define OPENMW_COMPONENTS_NIFOSG_SKELETON_H

#include <osg/Group>

#include <memory>

namespace SceneUtil
{

    /// @brief Defines a Bone hierarchy, used for updating of skeleton-space bone matrices.
    /// @note To prevent unnecessary updates, only bones that are used for skinning will be added to this hierarchy.
    class Bone
    {
    public:
        Bone();
        ~Bone();

        osg::Matrixf mMatrixInSkeletonSpace;

        osg::MatrixTransform* mNode;

        std::vector<Bone*> mChildren;

        /// Update the skeleton-space matrix of this bone and all its children.
        void update(const osg::Matrixf* parentMatrixInSkeletonSpace);

    private:
        Bone(const Bone&);
        void operator=(const Bone&);
    };

    /// @brief Handles the bone matrices for any number of child RigGeometries.
    /// @par Bones should be created as osg::MatrixTransform children of the skeleton.
    /// To be a referenced by a RigGeometry, a bone needs to have a unique name.
    class Skeleton : public osg::Group
    {
    public:
        Skeleton();
        Skeleton(const Skeleton& copy, const osg::CopyOp& copyop);

        META_Node(SceneUtil, Skeleton)

        /// Retrieve a bone by name.
        Bone* getBone(const std::string& name);

        /// Request an update of bone matrices. May be a no-op if already updated in this frame.
        void updateBoneMatrices(unsigned int traversalNumber);

        enum ActiveType
        {
            Inactive=0,
            SemiActive, /// Like Active, but don't bother with Update (including new bounding box) if we're off-screen
            Active
        };

        /// Set the skinning active flag. Inactive skeletons will not have their child rigs updated.
        /// You should set this flag to false if you know that bones are not currently moving.
        void setActive(ActiveType active);

        bool getActive() const;

        void traverse(osg::NodeVisitor& nv);

        void markDirty();

        virtual void childInserted(unsigned int);
        virtual void childRemoved(unsigned int, unsigned int);

    private:
        // The root bone is not a "real" bone, it has no corresponding node in the scene graph.
        // As far as the scene graph goes we support multiple root bones.
        std::unique_ptr<Bone> mRootBone;

        typedef std::map<std::string, std::pair<osg::NodePath, osg::MatrixTransform*> > BoneCache;
        BoneCache mBoneCache;
        bool mBoneCacheInit;

        bool mNeedToUpdateBoneMatrices;

        ActiveType mActive;

        unsigned int mLastFrameNumber;
        unsigned int mLastCullFrameNumber;
    };

}

#endif
