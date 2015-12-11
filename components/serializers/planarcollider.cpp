#include <boost/foreach.hpp>
#include <components/nifosg/particle.hpp>

#define SERIALIZER_DEBUG 0
#include "serializer.hpp"

REGISTER_OBJECT_WRAPPER2(NifOsg_PlanarCollider_Serializer,
                         new NifOsg::PlanarCollider,
                         NifOsg::PlanarCollider,
                         "OpenMW::PlanarCollider",
                         "osg::Object osgParticle::Operator OpenMW::PlanarCollider")
{
    SETUPMSG("OpenMW::PlanarCollider");
    ADD_FLOAT_SERIALIZER(BounceFactor, 0.0f);
    ADD_PLANE_SERIALIZER(Plane, osg::Plane());
    ADD_PLANE_SERIALIZER(PlaneInParticleSpace, osg::Plane());
}
