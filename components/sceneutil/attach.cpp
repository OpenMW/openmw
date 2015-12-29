#include "attach.hpp"

#include <stdexcept>

#include <osg/NodeVisitor>
#include <osg/Group>
#include <osg/Geode>
#include <osg/FrontFace>
#include <osg/PositionAttitudeTransform>
#include <osg/MatrixTransform>

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

        virtual void apply(osg::Node& node)
        {
            std::string lowerName = Misc::StringUtils::lowerCase(node.getName());
            if ((lowerName.size() >= mFilter.size() && lowerName.compare(0, mFilter.size(), mFilter) == 0)
                    || (lowerName.size() >= mFilter2.size() && lowerName.compare(0, mFilter2.size(), mFilter2) == 0))
            {
                mParent->addChild(&node);
            }
            else
                traverse(node);
        }

    private:
        osg::ref_ptr<osg::Group> mParent;
        std::string mFilter;
        std::string mFilter2;
    };

    osg::ref_ptr<osg::Node> attach(osg::ref_ptr<osg::Node> toAttach, osg::Node *master, const std::string &filter, const std::string &attachNode)
    {
        if (dynamic_cast<SceneUtil::Skeleton*>(toAttach.get()))
        {
            osg::ref_ptr<osg::Group> handle = new osg::Group;

            CopyRigVisitor copyVisitor(handle, filter);
            toAttach->accept(copyVisitor);

            master->asGroup()->addChild(handle);

            return handle;
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
                trans->setAttitude(osg::Quat(osg::DegreesToRadians(-90.f), osg::Vec3f(1,0,0)));
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
