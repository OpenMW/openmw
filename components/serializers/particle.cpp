//#define SERIALIZER_DEBUG

#ifdef SERIALIZER_DEBUG
#include <iostream>
#endif

#include <boost/foreach.hpp>

#include <osgDB/ObjectWrapper>
#include <osgDB/InputStream>
#include <osgDB/OutputStream>
#include <osgDB/Serializer>

#include <components/nifosg/nifloader.hpp>
#include <components/nifosg/particle.hpp>

#include "fixes.hpp"
#include "keymap.hpp"

#define MyClass NifOsg::ParticleSystem
REGISTER_OBJECT_WRAPPER(NifOsg_ParticleSystem_Serializer,
                        new NifOsg::ParticleSystem,
                        NifOsg::ParticleSystem,
                        "osg::Object osgParticle::ParticleSystem NifOsg::ParticleSystem")
{
#ifdef SERIALIZER_DEBUG
    std::cout << "Setting up NifOsg::ParticleSystem serializer..." << std::endl;
#endif
    ADD_INT_SERIALIZER(Quota, 0);
}

#undef MyClass
#define MyClass NifOsg::InverseWorldMatrix
REGISTER_OBJECT_WRAPPER(NifOsg_InverseWorldMatrix_Serializer,
                        new NifOsg::InverseWorldMatrix,
                        NifOsg::InverseWorldMatrix,
                        "osg::Object osg::NodeCallback NifOsg::InverseWorldMatrix")
{
#ifdef SERIALIZER_DEBUG
    std::cout << "Setting up NifOsg::InverseWorldMatrix serializer..." << std::endl;
#endif
    // There are no members in the InverseWorldMatrix callback.  It just needs to be created?
}

static bool checkColorData(const NifOsg::ParticleColorAffector& node) {
    if (node.mData.getMapTPtr() && node.mData.getMapTPtr()->mKeys.size() > 0) return true;
    return false;
}

static bool writeColorData(osgDB::OutputStream& os,
                           const NifOsg::ParticleColorAffector& node) {
#ifdef SERIALIZER_DEBUG
    std::cout << "Wrote NifOsg::ParticleColorAffector.mData" << std::endl;
#endif
    os << os.BEGIN_BRACKET << std::endl;
    os << os.PROPERTY("Interpolation") << node.mData.getMapTPtr()->mInterpolationType << std::endl;
    writeKeyMap<NifOsg::Vec4Interpolator::InnerMapType>(os, node.mData.getMapTPtr()->mKeys);
    os << os.END_BRACKET << std::endl;
    return true;
}

static bool readColorData(osgDB::InputStream& is,
                          NifOsg::ParticleColorAffector& node) {
#ifdef SERIALIZER_DEBUG
    std::cout << "Read NifOsg::ParticleColorAffector.mData" << std::endl;
#endif
    is >> is.BEGIN_BRACKET;
    node.mData.initMapTPtr();
    is >> is.PROPERTY("Interpolation") >> node.mData.getMapTPtr()->mInterpolationType;
    readKeyMap<NifOsg::Vec4Interpolator::InnerMapType, osg::Vec4f>(is, node.mData.getMapTPtr()->mKeys);
    is >> is.END_BRACKET;
    return true;
}

#undef MyClass
#define MyClass NifOsg::ParticleColorAffector
REGISTER_OBJECT_WRAPPER(NifOsg_ParticleColorAffector_Serializer,
                        new NifOsg::ParticleColorAffector,
                        NifOsg::ParticleColorAffector,
                        "osg::Object osgParticle::Operator NifOsg::ParticleColorAffector")
{
#ifdef SERIALIZER_DEBUG
    std::cout << "Setting up NifOsg::ParticleColorAffector serializer..." << std::endl;
#endif
    ADD_USER_SERIALIZER(ColorData); // Nif::NiColorData mData;
}

#undef MyClass
#define MyClass NifOsg::GrowFadeAffector
REGISTER_OBJECT_WRAPPER(NifOsg_GrowFadeAffector_Serializer,
                        new NifOsg::GrowFadeAffector,
                        NifOsg::GrowFadeAffector,
                        "osg::Object osgParticle::Operator NifOsg::GrowFadeAffector")
{
#ifdef SERIALIZER_DEBUG
    std::cout << "Setting up NifOsg::GrowFadeAffector serializer..." << std::endl;
#endif
    ADD_FLOAT_SERIALIZER(Grow, 0.0f);
    ADD_FLOAT_SERIALIZER(Fade, 0.0f);
    // No serialization for: float mCachedDefaultSize; Not necessary?
}

#undef MyClass
#define MyClass NifOsg::Emitter
REGISTER_OBJECT_WRAPPER(NifOsg_Emitter_Serializer,
                        new NifOsg::Emitter,
                        NifOsg::Emitter,
                        "osg::Object osgParticle::Emitter NifOsg::Emitter")
{
#ifdef SERIALIZER_DEBUG
    std::cout << "Setting up NifOsg::Emitter serializer..." << std::endl;
#endif
    ADD_LIST_SERIALIZER(Targets, std::vector<int>);
    ADD_OBJECT_SERIALIZER(Placer, osgParticle::Placer, NULL);
    ADD_OBJECT_SERIALIZER(Shooter, osgParticle::Shooter, NULL);
    ADD_OBJECT_SERIALIZER(Counter, osgParticle::Counter, NULL);
}

#undef MyClass
#define MyClass NifOsg::ParticleShooter
REGISTER_OBJECT_WRAPPER(NifOsg_ParticleShooter_Serializer,
                        new NifOsg::ParticleShooter,
                        NifOsg::ParticleShooter,
                        "osg::Object osgParticle::Shooter NifOsg::ParticleShooter")
{
#ifdef SERIALIZER_DEBUG
    std::cout << "Setting up NifOsg::ParticleShooter serializer..." << std::endl;
#endif
    ADD_FLOAT_SERIALIZER(MinSpeed, 0.0f);
    ADD_FLOAT_SERIALIZER(MaxSpeed, 0.0f);
    ADD_FLOAT_SERIALIZER(HorizontalDir, 0.0f);
    ADD_FLOAT_SERIALIZER(HorizontalAngle, 0.0f);
    ADD_FLOAT_SERIALIZER(VerticalDir, 0.0f);
    ADD_FLOAT_SERIALIZER(VerticalAngle, 0.0f);
    ADD_FLOAT_SERIALIZER(Lifetime, 0.0f);
    ADD_FLOAT_SERIALIZER(LifetimeRandom, 0.0f);
}

#undef MyClass
#define MyClass NifOsg::GravityAffector
REGISTER_OBJECT_WRAPPER(NifOsg_GravityAffector_Serializer,
                        new NifOsg::GravityAffector,
                        NifOsg::GravityAffector,
                        "osg::Object osgParticle::Operator NifOsg::GravityAffector")
{
#ifdef SERIALIZER_DEBUG
    std::cout << "Setting up NifOsg::GravityAffector serializer..." << std::endl;
#endif
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
