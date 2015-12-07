#include <components/sceneutil/statesetupdater.hpp>

#define SERIALIZER_DEBUG 0
#include "serializer.hpp"

REGISTER_OBJECT_WRAPPER2(NifOsg_StateSetUpdater_Serializer,
                         new SceneUtil::StateSetUpdater,
                         SceneUtil::StateSetUpdater,
                         "OpenMW::StateSetUpdater",
                         "osg::Object osg::NodeCallback OpenMW::StateSetUpdater")
{
    SETUPMSG("OpenMW::StateSetUpdater");
    // No serialization for: osg::ref_ptr<osg::StateSet> mStateSets[2];  Transient?
}
