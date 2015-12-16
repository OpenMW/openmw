#include <components/sceneutil/controller.hpp>

#define SERIALIZER_DEBUG 0
#include "serializer.hpp"

REGISTER_OBJECT_WRAPPER2(SceneUtil_ControllerSource_Serializer,
                         new SceneUtil::ControllerSource,
                         SceneUtil::ControllerSource,
                         "OpenMW::ControllerSource",
                         "osg::Object OpenMW::ControllerSource")
{
    SETUPMSG("OpenMW::ControllerSource");
}
