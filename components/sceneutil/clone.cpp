#include "clone.hpp"

#include <osg/StateSet>

#include <osgParticle/ParticleProcessor>
#include <osgParticle/ParticleSystemUpdater>
#include <osgParticle/Emitter>

#include <components/nifosg/userdata.hpp>

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

    osg::StateSet* CopyOp::operator ()(const osg::StateSet* stateset) const
    {
        if (!stateset)
            return nullptr;
        if (stateset->getDataVariance() == osg::StateSet::DYNAMIC)
            return osg::clone(stateset, *this);
        return const_cast<osg::StateSet*>(stateset);
    }

    osg::Object* CopyOp::operator ()(const osg::Object* node) const
    {
        // We should copy node transformations when we copy node
        if (const NifOsg::NodeUserData* data = dynamic_cast<const NifOsg::NodeUserData*>(node))
            return osg::clone(data, *this);

        return osg::CopyOp::operator()(node);
    }

    osg::Node* CopyOp::operator ()(const osg::Node* node) const
    {
        if (const osgParticle::ParticleProcessor* processor = dynamic_cast<const osgParticle::ParticleProcessor*>(node))
            return operator()(processor);
        if (const osgParticle::ParticleSystemUpdater* updater = dynamic_cast<const osgParticle::ParticleSystemUpdater*>(node))
        {
            osgParticle::ParticleSystemUpdater* cloned = new osgParticle::ParticleSystemUpdater(*updater, osg::CopyOp::SHALLOW_COPY);
            mMap2[cloned] = updater->getParticleSystem(0);
            return cloned;
        }
        return osg::CopyOp::operator()(node);
    }

    osg::Drawable* CopyOp::operator ()(const osg::Drawable* drawable) const
    {
        if (const osgParticle::ParticleSystem* partsys = dynamic_cast<const osgParticle::ParticleSystem*>(drawable))
            return operator()(partsys);

        if (dynamic_cast<const SceneUtil::RigGeometry*>(drawable) || dynamic_cast<const SceneUtil::MorphGeometry*>(drawable))
        {
            return osg::clone(drawable, *this);
        }

        return osg::CopyOp::operator()(drawable);
    }

    osgParticle::ParticleProcessor* CopyOp::operator() (const osgParticle::ParticleProcessor* processor) const
    {
        osgParticle::ParticleProcessor* cloned = osg::clone(processor, osg::CopyOp::DEEP_COPY_CALLBACKS);
        mMap[cloned] = processor->getParticleSystem();
        return cloned;
    }

    osgParticle::ParticleSystem* CopyOp::operator ()(const osgParticle::ParticleSystem* partsys) const
    {
        osgParticle::ParticleSystem* cloned = osg::clone(partsys, *this);

        for (std::map<osgParticle::ParticleProcessor*, const osgParticle::ParticleSystem*>::const_iterator it = mMap.begin(); it != mMap.end(); ++it)
        {
            if (it->second == partsys)
            {
                it->first->setParticleSystem(cloned);
            }
        }
        for (std::map<osgParticle::ParticleSystemUpdater*, const osgParticle::ParticleSystem*>::const_iterator it = mMap2.begin(); it != mMap2.end(); ++it)
        {
            if (it->second == partsys)
            {
                osgParticle::ParticleSystemUpdater* updater = it->first;
                updater->removeParticleSystem(updater->getParticleSystem(0));
                updater->addParticleSystem(cloned);
            }
        }
        return cloned;
    }

}
