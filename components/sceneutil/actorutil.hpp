#ifndef OPENMW_COMPONENTS_SCENEUTIL_ACTORUTIL_HPP
#define OPENMW_COMPONENTS_SCENEUTIL_ACTORUTIL_HPP

#include <string>

namespace SceneUtil
{
    const std::string& getActorSkeleton(bool firstPerson, bool female, bool beast, bool werewolf);
}

#endif
