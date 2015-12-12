#include <components/sceneutil/statesetupdater.hpp>

#define SERIALIZER_DEBUG 0
#include "serializer.hpp"

// No serialization is required for the only member which is a pair of StateSets, both of which are
// computed dynamically as a performance enhancement.

REGISTER_OBJECT_WRAPPER2(NifOsg_StateSetUpdater_Serializer,
                         new SceneUtil::StateSetUpdater,
                         SceneUtil::StateSetUpdater,
                         "OpenMW::StateSetUpdater",
                         "osg::Object osg::NodeCallback OpenMW::StateSetUpdater")
{
    SETUPMSG("OpenMW::StateSetUpdater");
}
