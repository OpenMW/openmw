#ifndef OPENMW_APPS_OPENMW_MWRENDER_ACTORUTIL_H
#define OPENMW_APPS_OPENMW_MWRENDER_ACTORUTIL_H

#include <string>

namespace MWRender
{
    const std::string& getActorSkeleton(bool firstPerson, bool female, bool beast, bool werewolf);
}

#endif
