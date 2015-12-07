#include <components/sceneutil/statesetupdater.hpp>

#define SERIALIZER_DEBUG 0
#include "serializer.hpp"

REGISTER_OBJECT_WRAPPER2(NifOsg_CompositeStateSetUpdater_Serializer,
                         new SceneUtil::CompositeStateSetUpdater,
                         SceneUtil::CompositeStateSetUpdater,
                         "OpenMW::CompositeStateSetUpdater",
                         "osg::Object osg::NodeCallback OpenMW::StateSetUpdater OpenMW::CompositeStateSetUpdater")
{
    SETUPMSG("OpenMW::CompositeStateSetUpdater");
    // No serialization for: std::vector<osg::ref_ptr<StateSetUpdater> > mCtrls;  Transient?
}
