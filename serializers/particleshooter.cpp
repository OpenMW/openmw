#include <components/nifosg/particle.hpp>

#define SERIALIZER_DEBUG 0
#include "serializer.hpp"

REGISTER_OBJECT_WRAPPER2(NifOsg_ParticleShooter_Serializer,
                         new NifOsg::ParticleShooter,
                         NifOsg::ParticleShooter,
                         "OpenMW::ParticleShooter",
                         "osg::Object osgParticle::Shooter OpenMW::ParticleShooter")
{
    SETUPMSG("OpenMW::ParticleShooter");
    ADD_FLOAT_SERIALIZER(MinSpeed, 0.0f);
    ADD_FLOAT_SERIALIZER(MaxSpeed, 0.0f);
    ADD_FLOAT_SERIALIZER(HorizontalDir, 0.0f);
    ADD_FLOAT_SERIALIZER(HorizontalAngle, 0.0f);
    ADD_FLOAT_SERIALIZER(VerticalDir, 0.0f);
    ADD_FLOAT_SERIALIZER(VerticalAngle, 0.0f);
    ADD_FLOAT_SERIALIZER(Lifetime, 0.0f);
    ADD_FLOAT_SERIALIZER(LifetimeRandom, 0.0f);
}
