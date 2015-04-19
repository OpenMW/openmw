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

#include <components/misc/stringops.hpp>

#include "visitor.hpp"

namespace SceneUtil
{

    class NodeMapVisitor : public osg::NodeVisitor
    {
    public:
        NodeMapVisitor() : osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN) {}

        void apply(osg::MatrixTransform& trans)
        {
            mMap[trans.getName()] = &trans;
            traverse(trans);
        }

        typedef std::map<std::string, osg::ref_ptr<osg::MatrixTransform> > NodeMap;

        const NodeMap& getNodeMap() const
        {
            return mMap;
        }

    private:
        NodeMap mMap;
    };

    /// Copy the matrix of a "source" node to a "dest" node (the node that the callback is attached to).
    /// Must be set on a MatrixTransform.
    class CopyController : public osg::NodeCallback
    {
    public:
        CopyController(osg::MatrixTransform* copyFrom)
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
            }

            traverse(node, nv);
        }

    private:
        const osg::MatrixTransform* mCopyFrom;
    };

    class AddCopyControllerVisitor : public osg::NodeVisitor
    {
    public:
        AddCopyControllerVisitor(const NodeMapVisitor::NodeMap& boneMap)
            : osg::NodeVisitor(TRAVERSE_ALL_CHILDREN)
            , mNodeMap(boneMap)
        {
        }

        virtual void apply(osg::MatrixTransform &node)
        {
            if (osgAnimation::Bone* bone = dynamic_cast<osgAnimation::Bone*>(&node))
            {
                NodeMapVisitor::NodeMap::const_iterator found = mNodeMap.find(bone->getName());
                if (found != mNodeMap.end())
                {
                    // add the CopyController at position 0 so it's executed before UpdateBone
                    osg::ref_ptr<osg::NodeCallback> old = bone->getUpdateCallback();
                    bone->setUpdateCallback(new CopyController(found->second.get()));
                    bone->addUpdateCallback(old);
                }
            }
            traverse(node);
        }

    private:
        const NodeMapVisitor::NodeMap& mNodeMap;
    };

    // FIXME: would be more efficient to copy only the wanted nodes instead of deleting unwanted ones later
    // copying is kinda cheap though, so don't bother for now
    class FilterVisitor : public osg::NodeVisitor
    {
    public:
        FilterVisitor(const std::string& filter)
            : osg::NodeVisitor(TRAVERSE_ALL_CHILDREN)
            , mFilter(Misc::StringUtils::lowerCase(filter))
        {
        }

        virtual void apply(osg::Geode &node)
        {
            std::string lowerName = Misc::StringUtils::lowerCase(node.getName());
            if (lowerName.find(mFilter) == std::string::npos)
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
        std::string mFilter;
    };

    osg::ref_ptr<osg::Node> attach(osg::ref_ptr<osg::Node> toAttach, osg::Node *master, const std::string &filter, const std::string &attachNode)
    {
        if (osgAnimation::Skeleton* skel = dynamic_cast<osgAnimation::Skeleton*>(toAttach.get()))
        {
            NodeMapVisitor nodeMapVisitor;
            master->accept(nodeMapVisitor);

            // would be more efficient if we could attach the RigGeometry directly to the master skeleton, but currently not possible
            // due to a difference in binding pose of the two skeletons
            AddCopyControllerVisitor visitor(nodeMapVisitor.getNodeMap());
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

            FindByNameVisitor findBoneOffset("BoneOffset");
            toAttach->accept(findBoneOffset);

            osg::ref_ptr<osg::PositionAttitudeTransform> trans;

            if (findBoneOffset.mFoundNode)
            {
                osg::MatrixTransform* boneOffset = dynamic_cast<osg::MatrixTransform*>(findBoneOffset.mFoundNode);
                if (!boneOffset)
                    throw std::runtime_error("BoneOffset must be a MatrixTransform");

                trans = new osg::PositionAttitudeTransform;
                trans->setPosition(boneOffset->getMatrix().getTrans());
                // The BoneOffset rotation seems to be incorrect
                trans->setAttitude(osg::Quat(-90, osg::Vec3f(1,0,0)));
            }

            if (attachNode.find("Left") != std::string::npos)
            {
                if (!trans)
                    trans = new osg::PositionAttitudeTransform;
                trans->setScale(osg::Vec3f(-1.f, 1.f, 1.f));

                // Need to invert culling because of the negative scale
                // Note: for absolute correctness we would need to check the current front face for every mesh then invert it
                // However MW isn't doing this either, so don't. Assuming all meshes are using backface culling is more efficient.
                osg::FrontFace* frontFace = new osg::FrontFace;
                frontFace->setMode(osg::FrontFace::CLOCKWISE);
                trans->getOrCreateStateSet()->setAttributeAndModes(frontFace, osg::StateAttribute::ON);
            }

            if (trans)
            {
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
