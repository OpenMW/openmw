#include "difficultyscaling.hpp"

#include <components/settings/settings.hpp>

/*
    Start of tes3mp addition

    Include additional headers for multiplayer purposes
*/
#include "../mwmp/Main.hpp"
#include "../mwmp/LocalPlayer.hpp"
/*
    End of tes3mp addition
*/

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"
#include "../mwworld/esmstore.hpp"

#include "actorutil.hpp"

float scaleDamage(float damage, const MWWorld::Ptr& attacker, const MWWorld::Ptr& victim)
{
    const MWWorld::Ptr& player = MWMechanics::getPlayer();

    // [-100, 100]
    int difficultySetting = Settings::Manager::getInt("difficulty", "Game");

    /*
        Start of tes3mp change (major)

        Use difficulty setting received from server instead of basing it on client settings
    */
    difficultySetting = mwmp::Main::get().getLocalPlayer()->difficulty;
    /*
        End of tes3mp change (major)
    */

    static const float fDifficultyMult = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("fDifficultyMult")->getFloat();

    float difficultyTerm = 0.01f * difficultySetting;

    float x = 0;
    if (victim == player)
    {
        if (difficultyTerm > 0)
            x = fDifficultyMult * difficultyTerm;
        else
            x = difficultyTerm / fDifficultyMult;
    }
    else if (attacker == player)
    {
        if (difficultyTerm > 0)
            x = -difficultyTerm / fDifficultyMult;
        else
            x = fDifficultyMult * (-difficultyTerm);
    }

    damage *= 1 + x;
    return damage;
}
