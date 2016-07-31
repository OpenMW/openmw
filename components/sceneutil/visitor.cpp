#include "visitor.hpp"

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

    void DisableFreezeOnCullVisitor::apply(osg::Drawable& drw)
    {
        if (osgParticle::ParticleSystem* partsys = dynamic_cast<osgParticle::ParticleSystem*>(&drw))
            partsys->setFreezeOnCull(false);
    }

}
