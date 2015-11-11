#include "visitor.hpp"

#include <osg/Geode>

#include <osgParticle/ParticleSystem>

#include <components/misc/stringops.hpp>

namespace SceneUtil
{

    void FindByNameVisitor::apply(osg::Group &group)
    {
        if (Misc::StringUtils::ciEqual(group.getName(), mNameToFind))
        {
            mFoundNode = &group;
            return;
        }
        traverse(group);
    }

    void DisableFreezeOnCullVisitor::apply(osg::Geode &geode)
    {
        for (unsigned int i=0; i<geode.getNumDrawables(); ++i)
            apply(*geode.getDrawable(i));
    }

    void DisableFreezeOnCullVisitor::apply(osg::Drawable& drw)
    {
        if (osgParticle::ParticleSystem* partsys = dynamic_cast<osgParticle::ParticleSystem*>(&drw))
            partsys->setFreezeOnCull(false);
    }

}
