#include "attach.hpp"

#include <stdexcept>

#include <osg/NodeVisitor>
#include <osg/Group>
#include <osg/Geometry>
#include <osg/FrontFace>
#include <osg/PositionAttitudeTransform>
#include <osg/MatrixTransform>

#include <components/debug/debuglog.hpp>
#include <components/misc/stringops.hpp>

#include <components/sceneutil/skeleton.hpp>
#include <components/resource/scenemanager.hpp>

#include "visitor.hpp"

namespace SceneUtil
{

    class CopyRigVisitor : public osg::NodeVisitor
    {
    public:
        CopyRigVisitor(osg::ref_ptr<osg::Group> parent, const std::string& filter)
            : osg::NodeVisitor(TRAVERSE_ALL_CHILDREN)
            , mParent(parent)
            , mFilter(Misc::StringUtils::lowerCase(filter))
        {
            mFilter2 = "tri " + mFilter;
        }

        void apply(osg::MatrixTransform& node) override
        {
            traverse(node);
        }
        void apply(osg::Node& node) override
        {
            traverse(node);
        }
        void apply(osg::Group& node) override
        {
            traverse(node);
        }

        void apply(osg::Drawable& drawable) override
        {
            if (!filterMatches(drawable.getName()))
                return;

            const osg::Node* node = &drawable;
            for (auto it = getNodePath().rbegin()+1; it != getNodePath().rend(); ++it)
            {
                const osg::Node* parent = *it;
                if (!filterMatches(parent->getName()))
                    break;
                node = parent;
            }
            mToCopy.emplace(node);
        }

        void doCopy(Resource::SceneManager* sceneManager)
        {
            for (const osg::ref_ptr<const osg::Node>& node : mToCopy)
            {
                mParent->addChild(sceneManager->getInstance(node));
            }
            mToCopy.clear();
        }

    private:

        bool filterMatches(const std::string& name) const
        {
            std::string lowerName = Misc::StringUtils::lowerCase(name);
            return (lowerName.size() >= mFilter.size() && lowerName.compare(0, mFilter.size(), mFilter) == 0)
                || (lowerName.size() >= mFilter2.size() && lowerName.compare(0, mFilter2.size(), mFilter2) == 0);
        }

        using NodeSet = std::set<osg::ref_ptr<const osg::Node>>;
        NodeSet mToCopy;

        osg::ref_ptr<osg::Group> mParent;
        std::string mFilter;
        std::string mFilter2;
    };

    void mergeUserData(const osg::UserDataContainer* source, osg::Object* target)
    {
        if (!source)
            return;
    
        if (!target->getUserDataContainer())
            target->setUserDataContainer(osg::clone(source, osg::CopyOp::SHALLOW_COPY));
        else
        {
            for (unsigned int i=0; i<source->getNumUserObjects(); ++i)
                target->getUserDataContainer()->addUserObject(osg::clone(source->getUserObject(i), osg::CopyOp::SHALLOW_COPY));
        }
    }

    osg::ref_ptr<osg::Node> attach(osg::ref_ptr<const osg::Node> toAttach, osg::Node *master, const std::string &filter, osg::Group* attachNode, Resource::SceneManager* sceneManager, const osg::Quat* attitude)
    {
        if (dynamic_cast<const SceneUtil::Skeleton*>(toAttach.get()))
        {
            osg::ref_ptr<osg::Group> handle = new osg::Group;

            CopyRigVisitor copyVisitor(handle, filter);
            const_cast<osg::Node*>(toAttach.get())->accept(copyVisitor);
            copyVisitor.doCopy(sceneManager);
            // add a ref to the original template to hint to the cache that it is still being used and should be kept in cache.
            handle->getOrCreateUserDataContainer()->addUserObject(new Resource::TemplateRef(toAttach));

            if (handle->getNumChildren() == 1)
            {
                osg::ref_ptr<osg::Node> newHandle = handle->getChild(0);
                handle->removeChild(newHandle);
                master->asGroup()->addChild(newHandle);
                mergeUserData(toAttach->getUserDataContainer(), newHandle);
                return newHandle;
            }
            else
            {
                master->asGroup()->addChild(handle);
                mergeUserData(toAttach->getUserDataContainer(), handle);
                return handle;
            }
        }
        else
        {
            osg::ref_ptr<osg::Node> clonedToAttach = sceneManager->getInstance(toAttach);

            FindByNameVisitor findBoneOffset("BoneOffset");
            clonedToAttach->accept(findBoneOffset);

            osg::ref_ptr<osg::PositionAttitudeTransform> trans;

            if (findBoneOffset.mFoundNode)
            {
                osg::MatrixTransform* boneOffset = dynamic_cast<osg::MatrixTransform*>(findBoneOffset.mFoundNode);
                if (!boneOffset)
                    throw std::runtime_error("BoneOffset must be a MatrixTransform");

                trans = new osg::PositionAttitudeTransform;
                trans->setPosition(boneOffset->getMatrix().getTrans());

                // Now that we used it, get rid of the redundant node.
                if (boneOffset->getNumChildren() == 0 && boneOffset->getNumParents() == 1)
                    boneOffset->getParent(0)->removeChild(boneOffset);
            }

            if (attachNode->getName().find("Left") != std::string::npos)
            {
                if (!trans)
                    trans = new osg::PositionAttitudeTransform;
                trans->setScale(osg::Vec3f(-1.f, 1.f, 1.f));

                // Need to invert culling because of the negative scale
                // Note: for absolute correctness we would need to check the current front face for every mesh then invert it
                // However MW isn't doing this either, so don't. Assuming all meshes are using backface culling is more efficient.
                static osg::ref_ptr<osg::StateSet> frontFaceStateSet;
                if (!frontFaceStateSet)
                {
                    frontFaceStateSet = new osg::StateSet;
                    osg::FrontFace* frontFace = new osg::FrontFace;
                    frontFace->setMode(osg::FrontFace::CLOCKWISE);
                    frontFaceStateSet->setAttributeAndModes(frontFace, osg::StateAttribute::ON);
                }
                trans->setStateSet(frontFaceStateSet);
            }

            if(attitude)
            {
                if (!trans)
                    trans = new osg::PositionAttitudeTransform;
                trans->setAttitude(*attitude);
            }

            if (trans)
            {
                attachNode->addChild(trans);
                trans->addChild(clonedToAttach);
                return trans;
            }
            else
            {
                attachNode->addChild(clonedToAttach);
                return clonedToAttach;
            }
        }
    }

}
