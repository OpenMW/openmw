#include <components/nifosg/particle.hpp>

#define SERIALIZER_DEBUG 0
#include "serializer.hpp"

REGISTER_OBJECT_WRAPPER2(NifOsg_ParticleSystem_Serializer,
                         new NifOsg::ParticleSystem,
                         NifOsg::ParticleSystem,
                         "OpenMW::ParticleSystem",
                         "osg::Object osgParticle::ParticleSystem OpenMW::ParticleSystem")
{
    SETUPMSG("OpenMW::ParticleSystem");
    ADD_INT_SERIALIZER(Quota, 0);
}
