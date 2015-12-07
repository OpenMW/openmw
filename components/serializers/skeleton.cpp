#include <components/sceneutil/skeleton.hpp>

#define SERIALIZER_DEBUG 0
#include "serializer.hpp"

REGISTER_OBJECT_WRAPPER2(SceneUtil_Skeleton_Serializer,
                         new SceneUtil::Skeleton,
                         SceneUtil::Skeleton,
                         "OpenMW::Skeleton",
                         "osg::Object osg::Group OpenMW::Skeleton")
{
    SETUPMSG("OpenMW::Skeleton");
    // INCOMPLETE
}
