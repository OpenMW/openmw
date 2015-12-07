#include <components/nifosg/particle.hpp>

#define SERIALIZER_DEBUG 0
#include "serializer.hpp"

REGISTER_OBJECT_WRAPPER2(NifOsg_GravityAffector_Serializer,
                         new NifOsg::GravityAffector,
                         NifOsg::GravityAffector,
                         "OpenMW::GravityAffector",
                         "osg::Object osgParticle::Operator OpenMW::GravityAffector")
{
    SETUPMSG("OpenMW::GravityAffector");
    ADD_FLOAT_SERIALIZER(Force, 0.0f);
    {
        typedef osgDB::EnumSerializer<NifOsg::GravityAffector,
                                      NifOsg::GravityAffector::ForceType, void> MySerializer;
        osg::ref_ptr<MySerializer> serializer = new MySerializer(
           "Type", NifOsg::GravityAffector::Type_Wind,
           &NifOsg::GravityAffector::getType,
           &NifOsg::GravityAffector::setType);
        serializer->add("Wind", NifOsg::GravityAffector::Type_Wind);
        serializer->add("Point", NifOsg::GravityAffector::Type_Point);
        wrapper->addSerializer(serializer.get(), osgDB::BaseSerializer::RW_ENUM);
    }
    ADD_VEC3F_SERIALIZER(Position, osg::Vec3f());
    ADD_VEC3F_SERIALIZER(Direction, osg::Vec3f());        
    ADD_FLOAT_SERIALIZER(Decay, 0.0f);
    // No serialization for: osg::Vec3f mCachedWorldPosition;  Transient?
    // No serialization for: osg::Vec3f mCachedWorldDirection;  Transient?
    //INCOMPLETE!!!
}
