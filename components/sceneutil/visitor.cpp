#include "visitor.hpp"

#include <osg/Drawable>
#include <osg/MatrixTransform>

#include <osgParticle/ParticleSystem>

#include <components/misc/stringops.hpp>

namespace SceneUtil
{

    bool FindByNameVisitor::checkGroup(osg::Group &group)
    {
        if (Misc::StringUtils::ciEqual(group.getName(), mNameToFind))
        {
            mFoundNode = &group;
            return true;
        }
        return false;
    }

    void FindByClassVisitor::apply(osg::Node &node)
    {
        if (Misc::StringUtils::ciEqual(node.className(), mNameToFind))
            mFoundNodes.push_back(&node);

        traverse(node);
    }

    void FindByNameVisitor::apply(osg::Group &group)
    {
        if (!checkGroup(group))
            traverse(group);
    }

    void FindByNameVisitor::apply(osg::MatrixTransform &node)
    {
        if (!checkGroup(node))
            traverse(node);
    }

    void FindByNameVisitor::apply(osg::Geometry&)
    {
    }

    void DisableFreezeOnCullVisitor::apply(osg::MatrixTransform &node)
    {
        traverse(node);
    }

    void DisableFreezeOnCullVisitor::apply(osg::Drawable& drw)
    {
        if (osgParticle::ParticleSystem* partsys = dynamic_cast<osgParticle::ParticleSystem*>(&drw))
            partsys->setFreezeOnCull(false);
    }

    void NodeMapVisitor::apply(osg::MatrixTransform& trans)
    {
        // Take transformation for first found node in file
        const std::string nodeName = Misc::StringUtils::lowerCase(trans.getName());
        if (mMap.find(nodeName) == mMap.end())
        {
            mMap[nodeName] = &trans;
        }

        traverse(trans);
    }

    void HideDrawablesVisitor::apply(osg::Drawable& drawable)
    {
        drawable.setNodeMask(0);
    }
}
