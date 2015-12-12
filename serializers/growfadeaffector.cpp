#include <components/nifosg/particle.hpp>

#define SERIALIZER_DEBUG 0
#include "serializer.hpp"

REGISTER_OBJECT_WRAPPER2(NifOsg_GrowFadeAffector_Serializer,
                         new NifOsg::GrowFadeAffector,
                         NifOsg::GrowFadeAffector,
                         "OpenMW::GrowFadeAffector",
                         "osg::Object osgParticle::Operator OpenMW::GrowFadeAffector")
{
    SETUPMSG("OpenMW::GrowFadeAffector");
    ADD_FLOAT_SERIALIZER(Grow, 0.0f);
    ADD_FLOAT_SERIALIZER(Fade, 0.0f);
    // No serialization is required for mCachedDefaultSize
}
