#include "actorutil.hpp"

#include <components/settings/settings.hpp>

namespace SceneUtil
{
    std::string getActorSkeleton(bool firstPerson, bool isFemale, bool isBeast, bool isWerewolf)
    {
        if (!firstPerson)
        {
            if (isWerewolf)
                return Settings::Manager::getString("wolfskin", "Models");
            else if (isBeast)
                return Settings::Manager::getString("baseanimkna", "Models");
            else if (isFemale)
                return Settings::Manager::getString("baseanimfemale", "Models");
            else
                return Settings::Manager::getString("baseanim", "Models");
        }
        else
        {
            if (isWerewolf)
                return Settings::Manager::getString("wolfskin1st", "Models");
            else if (isBeast)
                return Settings::Manager::getString("baseanimkna1st", "Models");
            else if (isFemale)
                return Settings::Manager::getString("baseanimfemale1st", "Models");
            else
                return Settings::Manager::getString("xbaseanim1st", "Models");
        }
    }
}
