// Virtual because of SceneUtil::Controller
#define OBJECT_CAST dynamic_cast

#include <components/nifosg/nifloader.hpp>

#define SERIALIZER_DEBUG 0
#include "serializer.hpp"
#include "keymap.hpp"

SERIALIZER_KEYMAPT(XRotations, NifOsg::FloatInterpolator::InnerMapType, float, NifOsg::KeyframeController)
SERIALIZER_KEYMAPT(YRotations, NifOsg::FloatInterpolator::InnerMapType, float, NifOsg::KeyframeController)
SERIALIZER_KEYMAPT(ZRotations, NifOsg::FloatInterpolator::InnerMapType, float, NifOsg::KeyframeController)
SERIALIZER_KEYMAPT(Scales, NifOsg::FloatInterpolator::InnerMapType, float, NifOsg::KeyframeController)
SERIALIZER_KEYMAPT(Rotations, NifOsg::QuaternionInterpolator::InnerMapType, osg::Quat, NifOsg::KeyframeController)
SERIALIZER_KEYMAPT(Translations, NifOsg::Vec3Interpolator::InnerMapType, osg::Vec3, NifOsg::KeyframeController)

REGISTER_OBJECT_WRAPPER2(NifOsg_KeyframeController_Serializer,
                         new NifOsg::KeyframeController,
                         NifOsg::KeyframeController,
                         "OpenMW::KeyframeController",
                         "osg::Object osg::NodeCallback OpenMW::KeyframeController")
{
    SETUPMSG("OpenMW::KeyframeController");
    ADD_USER_SERIALIZER(XRotations);
    ADD_USER_SERIALIZER(YRotations);
    ADD_USER_SERIALIZER(ZRotations);
    ADD_USER_SERIALIZER(Scales);
    ADD_USER_SERIALIZER(Rotations);
    ADD_USER_SERIALIZER(Translations);
}

