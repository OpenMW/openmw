#include <components/sceneutil/skeleton.hpp>

#define SERIALIZER_DEBUG 0
#include "serializer.hpp"

// No specific serialization is required for any members in Skeleton, since they're all computed
// dynamically.  The important part is to serialize the osg::Group, which contains the bones as
// children.

REGISTER_OBJECT_WRAPPER2(SceneUtil_Skeleton_Serializer,
                         new SceneUtil::Skeleton,
                         SceneUtil::Skeleton,
                         "OpenMW::Skeleton",
                         "osg::Object osg::Node osg::Group OpenMW::Skeleton")
{
    SETUPMSG("OpenMW::Skeleton");
}
