#include <components/sceneutil/controller.hpp>

#define SERIALIZER_DEBUG 0
#include "serializer.hpp"

REGISTER_OBJECT_WRAPPER2(SceneUtil_FrameTimeSource_Serializer,
                         new SceneUtil::FrameTimeSource,
                         SceneUtil::FrameTimeSource,
                         "OpenMW::FrameTimeSource",
                         "osg::Object OpenMW::ControllerSource OpenMW::FrameTimeSource")
{
    SETUPMSG("OpenMW::FrameTimeSource");
}
