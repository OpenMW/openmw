#include "sound.hpp"

#include <extern/sol2/sol.hpp>

#include "../luamanager.hpp"
#include "../luautil.hpp"

#include "../../mwbase/environment.hpp"
#include "../../mwbase/soundmanager.hpp"
#include "../../mwbase/windowmanager.hpp"
#include "../../mwbase/world.hpp"

#include "../../mwworld/class.hpp"
#include "../../mwworld/inventorystore.hpp"

namespace MWLua
{
    void bindTES3Sound()
    {
        auto stateHandle = LuaManager::getInstance().getThreadSafeStateHandle();
        sol::state& state = stateHandle.state;

        state["omw"]["playSound"] = [](sol::optional<sol::table> params)
        {
            // Get parameters.
            const char* sound = getOptionalParam<const char*>(params, "sound", nullptr);
            MWWorld::Ptr ptr = getOptionalParamReference(params, "reference");
            bool loop = getOptionalParam<bool>(params, "loop", false);
            double volume = getOptionalParam<double>(params, "volume", 1.0);
            float pitch = getOptionalParam<double>(params, "pitch", 1.0);

            // Clamp volume. RIP no std::clamp.
            volume = std::max(0.0, volume);
            volume = std::min(volume, 1.0);

            if (ptr.isEmpty())
            {
                MWBase::Environment::get().getSoundManager()->playSound(sound, volume, pitch, MWSound::Type::Sfx, MWSound::PlayMode::NoEnv);
            }
            else
            {
                MWBase::Environment::get().getSoundManager()->playSound3D(ptr, sound, volume, pitch,
                                                                            MWSound::Type::Sfx,
                                                                            loop ? MWSound::PlayMode::LoopRemoveAtDistance
                                                                                : MWSound::PlayMode::Normal);
            }
        };

        state["omw"]["getSoundPlaying"] = [](sol::optional<sol::table> params)
        {
            MWWorld::Ptr ptr = getOptionalParamReference(params, "reference");
            const char* sound = getOptionalParam<const char*>(params, "sound", nullptr);

            bool ret = MWBase::Environment::get().getSoundManager()->getSoundPlaying (ptr, sound);

            // GetSoundPlaying called on an equipped item should also look for sounds played by the equipping actor.
            if (!ret && ptr.getContainerStore())
            {
                MWWorld::Ptr cont = MWBase::Environment::get().getWorld()->findContainer(ptr);

                if (!cont.isEmpty() && cont.getClass().hasInventoryStore(cont) && cont.getClass().getInventoryStore(cont).isEquipped(ptr))
                {
                    ret = MWBase::Environment::get().getSoundManager()->getSoundPlaying (cont, sound);
                }
            }

            return ret;
        };

        state["omw"]["streamMusic"] = [](sol::optional<sol::table> params)
        {
            // Get parameters.
            // FIXME: support for MCP's uninterruptable and crossfade
            //int situation = getOptionalParam<int>(params, "situation", int(TES3::MusicSituation::Uninterruptible));
            //double crossfade = getOptionalParam<double>(params, "crossfade", 1.0);
            const char* path = getOptionalParam<const char*>(params, "path", nullptr);
            MWBase::Environment::get().getSoundManager()->streamMusic (path);
        };

        state["omw"]["playItemPickupSound"] = [](sol::optional<sol::table> params)
        {
            MWWorld::Ptr ptr = getOptionalParamReference(params, "reference");
            MWWorld::Ptr item = getOptionalParamReference(params, "item");
            if (item.isEmpty())
                return;

            bool pickup = getOptionalParam<bool>(params, "pickup", true);
            std::string soundId;
            if (pickup)
            {
                soundId = item.getClass().getUpSoundId(item);
            }
            else
            {
                soundId = item.getClass().getDownSoundId(item);
            }

            if (soundId.empty())
                return;

            if (ptr.isEmpty())
            {
                MWBase::Environment::get().getWindowManager()->playSound(soundId);
            }
            else
            {
                MWBase::Environment::get().getSoundManager()->playSound3D(ptr, soundId, 1.0f, 1.0f, MWSound::Type::Sfx, MWSound::PlayMode::Normal);
            }
        };

        state["omw"]["say"] = [](sol::table params)
        {
            MWWorld::Ptr ptr = getOptionalParamExecutionReference(params);
            if (ptr.isEmpty())
            {
                throw std::invalid_argument("Invalid reference parameter provided.");
            }

            const char* path = getOptionalParam<const char*>(params, "soundPath", nullptr);
            if (path == nullptr)
            {
                throw std::invalid_argument("Invalid soundPath parameter provided.");
            }

            // FIXME: optional volume parameters
            //float pitch = getOptionalParam(params, "pitch", 1.0f);
            //float volume = std::min(std::max(0.0f, getOptionalParam(params, "volume", 1.0f)), 1.0f);

            MWBase::Environment::get().getSoundManager()->say (ptr, path);

            if (MWBase::Environment::get().getWindowManager()->getSubtitlesEnabled() || getOptionalParam(params, "forceSubtitle", false))
            {
                const char* subtitle = getOptionalParam<const char*>(params, "subtitle", nullptr);
                if (subtitle != nullptr)
                    MWBase::Environment::get().getWindowManager()->messageBox (subtitle);
            }
        };
    }
}
