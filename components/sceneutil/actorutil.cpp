#include "actorutil.hpp"

#include <components/settings/values.hpp>

namespace SceneUtil
{
    const std::string& getActorSkeleton(bool firstPerson, bool isFemale, bool isBeast, bool isWerewolf)
    {
        if (!firstPerson)
        {
            if (isWerewolf)
                return Settings::models().mWolfskin;
            else if (isBeast)
                return Settings::models().mBaseanimkna;
            else if (isFemale)
                return Settings::models().mBaseanimfemale;
            else
                return Settings::models().mBaseanim;
        }
        else
        {
            if (isWerewolf)
                return Settings::models().mWolfskin1st;
            else if (isBeast)
                return Settings::models().mBaseanimkna1st;
            else if (isFemale)
                return Settings::models().mBaseanimfemale1st;
            else
                return Settings::models().mXbaseanim1st;
        }
    }
}
