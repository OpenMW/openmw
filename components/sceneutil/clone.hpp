#ifndef OPENMW_COMPONENTS_SCENEUTIL_CLONE_H
#define OPENMW_COMPONENTS_SCENEUTIL_CLONE_H

#include <map>

#include <osg/CopyOp>

namespace osgParticle
{
    class ParticleProcessor;
    class ParticleSystem;
    class ParticleSystemUpdater;
}

namespace SceneUtil
{

    /// @par Defines the cloning behaviour we need:
    /// * Assigns updated ParticleSystem pointers on cloned emitters and programs.
    /// * Creates deep copy of StateSets if they have a DYNAMIC data variance.
    /// * Deep copies RigGeometry and MorphGeometry so they can animate without affecting clones.
    /// @warning Do not use an object of this class for more than one copy operation.
    class CopyOp : public osg::CopyOp
    {
    public:
        CopyOp();

        virtual osgParticle::ParticleSystem* operator() (const osgParticle::ParticleSystem* partsys) const;
        virtual osgParticle::ParticleProcessor* operator() (const osgParticle::ParticleProcessor* processor) const;

        virtual osg::Node* operator() (const osg::Node* node) const;
        virtual osg::Drawable* operator() (const osg::Drawable* drawable) const;

        virtual osg::StateSet* operator() (const osg::StateSet* stateset) const;
        virtual osg::Object* operator ()(const osg::Object* node) const;

    private:
        // maps new pointers to their old pointers
        // a little messy, but I think this should be the most efficient way
        mutable std::map<osgParticle::ParticleProcessor*, const osgParticle::ParticleSystem*> mProcessorToOldPs;
        mutable std::map<osgParticle::ParticleSystemUpdater*, const osgParticle::ParticleSystem*> mUpdaterToOldPs;
        mutable std::map<const osgParticle::ParticleSystem*, osgParticle::ParticleSystem*> mOldPsToNewPs;
    };

}

#endif
