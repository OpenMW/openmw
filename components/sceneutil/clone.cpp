#include "clone.hpp"

#include <osg/StateSet>

#include <osgAnimation/Bone>
#include <osgAnimation/Skeleton>
#include <osgAnimation/MorphGeometry>
#include <osgAnimation/RigGeometry>

#include <osgParticle/ParticleProcessor>
#include <osgParticle/ParticleSystemUpdater>
#include <osgParticle/Emitter>

#include <components/sceneutil/morphgeometry.hpp>
#include <components/sceneutil/riggeometry.hpp>

namespace SceneUtil
{

    CopyOp::CopyOp()
    {
        setCopyFlags(osg::CopyOp::DEEP_COPY_NODES
                     // Controller might need different inputs per scene instance
                     | osg::CopyOp::DEEP_COPY_CALLBACKS
                     | osg::CopyOp::DEEP_COPY_USERDATA);
    }

    osg::Node* CopyOp::operator ()(const osg::Node* node) const
    {
        if (const osgParticle::ParticleProcessor* processor = dynamic_cast<const osgParticle::ParticleProcessor*>(node))
            return operator()(processor);
        if (const osgParticle::ParticleSystemUpdater* updater = dynamic_cast<const osgParticle::ParticleSystemUpdater*>(node))
        {
            osgParticle::ParticleSystemUpdater* cloned = new osgParticle::ParticleSystemUpdater(*updater, osg::CopyOp::SHALLOW_COPY);
            mUpdaterToOldPs[cloned] = updater->getParticleSystem(0);
            return cloned;
        }

        if (dynamic_cast<const osgAnimation::Bone*>(node) || dynamic_cast<const osgAnimation::Skeleton*>(node))
        {
            return osg::clone(node, *this);
        }
        return osg::CopyOp::operator()(node);
    }

    osg::Drawable* CopyOp::operator ()(const osg::Drawable* drawable) const
    {
        if (const osgParticle::ParticleSystem* partsys = dynamic_cast<const osgParticle::ParticleSystem*>(drawable))
            return operator()(partsys);

        if (dynamic_cast<const SceneUtil::RigGeometry*>(drawable) || dynamic_cast<const SceneUtil::MorphGeometry*>(drawable) || dynamic_cast<const osgAnimation::RigGeometry*>(drawable) || dynamic_cast<const osgAnimation::MorphGeometry*>(drawable))
        {
            return static_cast<osg::Drawable*>(drawable->clone(*this));
        }

        return osg::CopyOp::operator()(drawable);
    }

    osgParticle::ParticleProcessor* CopyOp::operator() (const osgParticle::ParticleProcessor* processor) const
    {
        osgParticle::ParticleProcessor* cloned = static_cast<osgParticle::ParticleProcessor*>(processor->clone(osg::CopyOp::DEEP_COPY_CALLBACKS));
        for (const auto& oldPsNewPsPair : mOldPsToNewPs)
        {
            if (processor->getParticleSystem() == oldPsNewPsPair.first)
            {
                cloned->setParticleSystem(oldPsNewPsPair.second);
                return cloned;
            }
        }

        mProcessorToOldPs[cloned] = processor->getParticleSystem();
        return cloned;
    }

    osgParticle::ParticleSystem* CopyOp::operator ()(const osgParticle::ParticleSystem* partsys) const
    {
        osgParticle::ParticleSystem* cloned = static_cast<osgParticle::ParticleSystem*>(partsys->clone(*this));

        for (const auto& processorPsPair : mProcessorToOldPs)
        {
            if (processorPsPair.second == partsys)
            {
                processorPsPair.first->setParticleSystem(cloned);
            }
        }
        for (const auto& updaterPsPair : mUpdaterToOldPs)
        {
            if (updaterPsPair.second == partsys)
            {
                osgParticle::ParticleSystemUpdater* updater = updaterPsPair.first;
                updater->removeParticleSystem(updater->getParticleSystem(0));
                updater->addParticleSystem(cloned);
            }
        }
        // In rare situations a particle processor may be placed after the particle system in the scene graph.
        mOldPsToNewPs[partsys] = cloned;

        return cloned;
    }

}
