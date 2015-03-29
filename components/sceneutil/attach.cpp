#include "attach.hpp"

#include <stdexcept>
#include <iostream>

#include <osg/NodeVisitor>
#include <osg/Group>
#include <osg/Geode>
#include <osg/FrontFace>
#include <osg/PositionAttitudeTransform>

#include <osgAnimation/BoneMapVisitor>
#include <osgAnimation/Skeleton>

namespace SceneUtil
{

    class FindByNameVisitor : public osg::NodeVisitor
    {
    public:
        FindByNameVisitor(const std::string& nameToFind)
            : osg::NodeVisitor(TRAVERSE_ALL_CHILDREN)
            , mNameToFind(nameToFind)
            , mFoundNode(NULL)
        {
        }

        virtual void apply(osg::Node &node)
        {
            osg::Group* group = node.asGroup();
            if (group && node.getName() == mNameToFind)
            {
                mFoundNode = group;
                return;
            }
            traverse(node);
        }

        const std::string& mNameToFind;
        osg::Group* mFoundNode;
    };

    /// Copy the skeleton-space matrix of a "source" bone to a "dest" bone (the bone that the callback is attached to).
    /// Must be set on a Bone.
    class CopyController : public osg::NodeCallback
    {
    public:
        CopyController(osgAnimation::Bone* copyFrom)
            : mCopyFrom(copyFrom)
        {
        }
        CopyController(const CopyController& copy, const osg::CopyOp& copyop)
            : osg::NodeCallback(copy, copyop)
            , mCopyFrom(copy.mCopyFrom)
        {
        }
        CopyController()
            : mCopyFrom(NULL)
        {
        }

        virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
        {
            osgAnimation::Bone* bone = static_cast<osgAnimation::Bone*>(node);

            if (mCopyFrom)
            {
                bone->setMatrix(mCopyFrom->getMatrix());
                bone->setMatrixInSkeletonSpace(mCopyFrom->getMatrixInSkeletonSpace());
            }

            traverse(node, nv);
        }

    private:
        const osgAnimation::Bone* mCopyFrom;
    };

    class AddCopyControllerVisitor : public osg::NodeVisitor
    {
    public:
        AddCopyControllerVisitor(const osgAnimation::BoneMap& boneMap)
            : osg::NodeVisitor(TRAVERSE_ALL_CHILDREN)
            , mBoneMap(boneMap)
        {
        }

        virtual void apply(osg::MatrixTransform &node)
        {
            if (osgAnimation::Bone* bone = dynamic_cast<osgAnimation::Bone*>(&node))
            {
                osgAnimation::BoneMap::const_iterator found = mBoneMap.find(bone->getName());
                if (found != mBoneMap.end())
                {
                    bone->setUpdateCallback(new CopyController(found->second.get()));
                }
            }
        }

    private:
        const osgAnimation::BoneMap& mBoneMap;
    };

    // FIXME: would be more efficient to copy only the wanted nodes instead of deleting unwanted ones later
    class FilterVisitor : public osg::NodeVisitor
    {
    public:
        FilterVisitor(const std::string& filter)
            : osg::NodeVisitor(TRAVERSE_ALL_CHILDREN)
            , mFilter(filter)
        {
        }

        virtual void apply(osg::Geode &node)
        {
            if (node.getName().find(mFilter) == std::string::npos)
            {
                mToRemove.push_back(&node);
            }
        }

        void removeFilteredParts()
        {
            for (std::vector<osg::Geode*>::iterator it = mToRemove.begin(); it != mToRemove.end(); ++it)
            {
                osg::Geode* geode = *it;
                geode->getParent(0)->removeChild(geode);
            }
        }

    private:
        std::vector<osg::Geode*> mToRemove;
        const std::string& mFilter;
    };

    osg::ref_ptr<osg::Node> attach(osg::ref_ptr<osg::Node> toAttach, osg::Node *master, const std::string &filter, const std::string &attachNode)
    {
        if (osgAnimation::Skeleton* skel = dynamic_cast<osgAnimation::Skeleton*>(toAttach.get()))
        {
            osgAnimation::Skeleton* masterSkel = dynamic_cast<osgAnimation::Skeleton*>(master);
            osgAnimation::BoneMapVisitor boneMapVisitor;
            masterSkel->accept(boneMapVisitor);

            AddCopyControllerVisitor visitor(boneMapVisitor.getBoneMap());
            toAttach->accept(visitor);

            FilterVisitor filterVisitor(filter);
            toAttach->accept(filterVisitor);
            filterVisitor.removeFilteredParts();

            master->asGroup()->addChild(skel);
            return skel;
        }
        else
        {
            FindByNameVisitor find(attachNode);
            master->accept(find);
            if (!find.mFoundNode)
                throw std::runtime_error(std::string("Can't find attachment node ") + attachNode);

            if (attachNode.find("Left") != std::string::npos)
            {
                osg::ref_ptr<osg::PositionAttitudeTransform> trans = new osg::PositionAttitudeTransform;
                trans->setScale(osg::Vec3f(-1.f, 1.f, 1.f));

                // Need to invert culling because of the negative scale
                // Note: for absolute correctness we would need to check the current front face for every mesh then invert it
                // However MW isn't doing this either, so don't. Assuming all meshes are using backface culling is more efficient.
                osg::FrontFace* frontFace = new osg::FrontFace;
                frontFace->setMode(osg::FrontFace::CLOCKWISE);
                toAttach->getOrCreateStateSet()->setAttributeAndModes(frontFace, osg::StateAttribute::ON);

                find.mFoundNode->addChild(trans);
                trans->addChild(toAttach);
                return trans;
            }
            else
            {
                find.mFoundNode->addChild(toAttach);
                return toAttach;
            }
        }
    }

}
