#ifndef GAME_MWMECHANICS_AIPACKAGETYPEID_H
#define GAME_MWMECHANICS_AIPACKAGETYPEID_H

namespace MWMechanics
{
    ///Enumerates the various AITypes available
    enum class AiPackageTypeId
    {
        None = -1,
        Wander = 0,
        Travel = 1,
        Escort = 2,
        Follow = 3,
        Activate = 4,

        // These 5 are not really handled as Ai Packages in the MW engine
        // For compatibility do *not* return these in the getCurrentAiPackage script function..
        Combat = 5,
        Pursue = 6,
        AvoidDoor = 7,
        Face = 8,
        Breathe = 9,
        InternalTravel = 10,
        Cast = 11
    };
}

#endif
