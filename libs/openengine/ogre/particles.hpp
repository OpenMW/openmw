#ifndef OENGINE_OGRE_PARTICLES_H
#define OENGINE_OGRE_PARTICLES_H

#include <OgreParticleAffectorFactory.h>

/** Factory class for GrowFadeAffector. */
class GrowFadeAffectorFactory : public Ogre::ParticleAffectorFactory
{
    /** See Ogre::ParticleAffectorFactory */
    Ogre::String getName() const
    { return "GrowFade"; }

    /** See Ogre::ParticleAffectorFactory */
    Ogre::ParticleAffector *createAffector(Ogre::ParticleSystem *psys);
};

/** Factory class for GravityAffector. */
class GravityAffectorFactory : public Ogre::ParticleAffectorFactory
{
    /** See Ogre::ParticleAffectorFactory */
    Ogre::String getName() const
    { return "Gravity"; }

    /** See Ogre::ParticleAffectorFactory */
    Ogre::ParticleAffector *createAffector(Ogre::ParticleSystem *psys);
};

#endif /* OENGINE_OGRE_PARTICLES_H */
