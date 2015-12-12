#include <components/nifosg/particle.hpp>

#define SERIALIZER_DEBUG 0
#include "serializer.hpp"
#include "keymap.hpp"

SERIALIZER_KEYMAPT(Data, NifOsg::Vec4Interpolator::InnerMapType, osg::Vec4f, NifOsg::ParticleColorAffector)

REGISTER_OBJECT_WRAPPER2(NifOsg_ParticleColorAffector_Serializer,
                         new NifOsg::ParticleColorAffector,
                         NifOsg::ParticleColorAffector,
                         "OpenMW::ParticleColorAffector",
                         "osg::Object osgParticle::Operator OpenMW::ParticleColorAffector")
{
    SETUPMSG("OpenMW::ParticleColorAffector");
    ADD_USER_SERIALIZER(Data);
}
