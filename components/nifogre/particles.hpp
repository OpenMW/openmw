#ifndef OENGINE_OGRE_PARTICLES_H
#define OENGINE_OGRE_PARTICLES_H

#include <OgreParticleEmitterFactory.h>
#include <OgreParticleAffectorFactory.h>

/** Factory class for NifEmitter. */
class NifEmitterFactory : public Ogre::ParticleEmitterFactory
{
public:
    /** See ParticleEmitterFactory */
    Ogre::String getName() const
    { return "Nif"; }

    /** See ParticleEmitterFactory */
    Ogre::ParticleEmitter* createEmitter(Ogre::ParticleSystem *psys);
};

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

struct NiNodeHolder
{
    std::vector<Ogre::Bone*> mBones;

    // Ogre::Any needs this for some reason
    friend std::ostream& operator<<(std::ostream& o, const NiNodeHolder& r)
    { return o; }
};

#endif /* OENGINE_OGRE_PARTICLES_H */
