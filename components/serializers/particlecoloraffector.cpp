#include <components/nifosg/particle.hpp>

#define SERIALIZER_DEBUG 0
#include "serializer.hpp"
#include "keymap.hpp"

static bool checkColorData(const NifOsg::ParticleColorAffector& node) {
    CHECKMSG("Data");
    if (node.mData.getMapTPtr() && node.mData.getMapTPtr()->mKeys.size() > 0) return true;
    return false;
}

static bool writeColorData(osgDB::OutputStream& os,
                           const NifOsg::ParticleColorAffector& node) {
    WRITEMSG("Data");
    os << os.BEGIN_BRACKET << std::endl;
    os << os.PROPERTY("Interpolation") << node.mData.getMapTPtr()->mInterpolationType << std::endl;
    writeKeyMap<NifOsg::Vec4Interpolator::InnerMapType>(os, node.mData.getMapTPtr()->mKeys);
    os << os.END_BRACKET << std::endl;
    return true;
}

static bool readColorData(osgDB::InputStream& is,
                          NifOsg::ParticleColorAffector& node) {
    READMSG("Data");
    is >> is.BEGIN_BRACKET;
    node.mData.initMapTPtr();
    is >> is.PROPERTY("Interpolation") >> node.mData.getMapTPtr()->mInterpolationType;
    readKeyMap<NifOsg::Vec4Interpolator::InnerMapType, osg::Vec4f>(is, node.mData.getMapTPtr()->mKeys);
    is >> is.END_BRACKET;
    return true;
}

REGISTER_OBJECT_WRAPPER2(NifOsg_ParticleColorAffector_Serializer,
                         new NifOsg::ParticleColorAffector,
                         NifOsg::ParticleColorAffector,
                         "OpenMW::ParticleColorAffector",
                         "osg::Object osgParticle::Operator OpenMW::ParticleColorAffector")
{
    SETUPMSG("OpenMW::ParticleColorAffector");
    ADD_USER_SERIALIZER(ColorData); // Nif::NiColorData mData;
}
