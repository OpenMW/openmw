#include <components/sceneutil/controller.hpp>

#define SERIALIZER_DEBUG 0
#include "serializer.hpp"

REGISTER_OBJECT_WRAPPER2(SceneUtil_ControllerFunction_Serializer,
                         new SceneUtil::ControllerFunction,
                         SceneUtil::ControllerFunction,
                         "OpenMW::ControllerFunction",
                         "osg::Object OpenMW::ControllerFunction")
{
    SETUPMSG("OpenMW::ControllerFunction");
}
