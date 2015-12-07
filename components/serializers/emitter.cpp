#include <components/nifosg/particle.hpp>

#define SERIALIZER_DEBUG 0
#include "serializer.hpp"

REGISTER_OBJECT_WRAPPER2(NifOsg_Emitter_Serializer,
                         new NifOsg::Emitter,
                         NifOsg::Emitter,
                         "OpenMW::Emitter",
                         "osg::Object osgParticle::Emitter OpenMW::Emitter")
{
    SETUPMSG("OpenMW::Emitter");
    ADD_LIST_SERIALIZER(Targets, std::vector<int>);
    ADD_OBJECT_SERIALIZER(Placer, osgParticle::Placer, NULL);
    ADD_OBJECT_SERIALIZER(Shooter, osgParticle::Shooter, NULL);
    ADD_OBJECT_SERIALIZER(Counter, osgParticle::Counter, NULL);
}
