#include "visitor.hpp"

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

}
