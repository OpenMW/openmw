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

        virtual void apply(osg::MatrixTransform& node)
        {
            traverse(node);
        }
        virtual void apply(osg::Node& node)
        {
            traverse(node);
        }
        virtual void apply(osg::Group& node)
        {
            traverse(node);
        }

        virtual void apply(osg::Drawable& drawable)
        {
            std::string lowerName = Misc::StringUtils::lowerCase(drawable.getName());
            if ((lowerName.size() >= mFilter.size() && lowerName.compare(0, mFilter.size(), mFilter) == 0)
                    || (lowerName.size() >= mFilter2.size() && lowerName.compare(0, mFilter2.size(), mFilter2) == 0))
            {
                osg::Node* node = &drawable;
                while (node && node->getNumParents() && !node->getStateSet())
                    node = node->getParent(0);
                if (node)
                    mToCopy.push_back(node);
            }
        }

        void doCopy()
        {
            for (std::vector<osg::ref_ptr<osg::Node> >::iterator it = mToCopy.begin(); it != mToCopy.end(); ++it)
            {
                osg::ref_ptr<osg::Node> node = *it;
                if (node->getNumParents() > 1)
                    Log(Debug::Error) << "Error CopyRigVisitor: node has multiple parents";
                while (node->getNumParents())
                    node->getParent(0)->removeChild(node);

                mParent->addChild(node);
            }
            mToCopy.clear();
        }

    private:
        typedef std::vector<osg::ref_ptr<osg::Node> > NodeVector;
        NodeVector mToCopy;

        osg::ref_ptr<osg::Group> mParent;
        std::string mFilter;
        std::string mFilter2;
    };

    void mergeUserData(osg::UserDataContainer* source, osg::Object* target)
    {
        if (!target->getUserDataContainer())
            target->setUserDataContainer(source);
        else
        {
            for (unsigned int i=0; i<source->getNumUserObjects(); ++i)
                target->getUserDataContainer()->addUserObject(source->getUserObject(i));
        }
    }

    osg::ref_ptr<osg::Node> attach(osg::ref_ptr<osg::Node> toAttach, osg::Node *master, const std::string &filter, osg::Group* attachNode)
    {
        if (dynamic_cast<SceneUtil::Skeleton*>(toAttach.get()))
        {
            osg::ref_ptr<osg::Group> handle = new osg::Group;

            CopyRigVisitor copyVisitor(handle, filter);
            toAttach->accept(copyVisitor);
            copyVisitor.doCopy();

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
                handle->setUserDataContainer(toAttach->getUserDataContainer());
                return handle;
            }
        }
        else
        {
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
                trans->setAttitude(osg::Quat(osg::DegreesToRadians(-90.f), osg::Vec3f(1,0,0)));

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

            if (trans)
            {
                attachNode->addChild(trans);
                trans->addChild(toAttach);
                return trans;
            }
            else
            {
                attachNode->addChild(toAttach);
                return toAttach;
            }
        }
    }

}
