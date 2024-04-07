#ifndef OPENMW_APPS_OPENMW_MWRENDER_ACTORUTIL_H
#define OPENMW_APPS_OPENMW_MWRENDER_ACTORUTIL_H

#include <string>
#include <string_view>

namespace MWRender
{
    const std::string& getActorSkeleton(bool firstPerson, bool female, bool beast, bool werewolf);
    bool isDefaultActorSkeleton(std::string_view model);
    std::string addSuffixBeforeExtension(const std::string& filename, const std::string& suffix);
}

#endif
