// Virtual because of SceneUtil::StateSetUpdater
#define OBJECT_CAST dynamic_cast

#include <boost/foreach.hpp>
#include <components/nifosg/controller.hpp>

#define SERIALIZER_DEBUG 0
#include "serializer.hpp"
#include "keymap.hpp"

SERIALIZER_KEYMAPT(Data, NifOsg::Vec3Interpolator::InnerMapType, osg::Vec3f, NifOsg::MaterialColorController)

REGISTER_OBJECT_WRAPPER2(NifOsg_MaterialColorController_Serializer,
                         new NifOsg::MaterialColorController,
                         NifOsg::MaterialColorController,
                         "OpenMW::MaterialColorController",
                         "osg::Object osg::NodeCallback OpenMW::StateSetUpdater OpenMW::MaterialColorController")
{
    SETUPMSG("OpenMW::MaterialColorController");
    ADD_USER_SERIALIZER(Data);
}
