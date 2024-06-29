#include "menuscripts.hpp"

#include <components/lua/util.hpp>
#include <components/misc/strings/lower.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/statemanager.hpp"
#include "../mwstate/character.hpp"

namespace MWLua
{
    static const MWState::Character* findCharacter(std::string_view characterDir)
    {
        MWBase::StateManager* manager = MWBase::Environment::get().getStateManager();
        for (auto it = manager->characterBegin(); it != manager->characterEnd(); ++it)
            if (it->getPath().filename() == characterDir)
                return &*it;
        return nullptr;
    }

    static const MWState::Slot* findSlot(const MWState::Character* character, std::string_view slotName)
    {
        if (!character)
            return nullptr;
        for (const MWState::Slot& slot : *character)
            if (slot.mPath.filename() == slotName)
                return &slot;
        return nullptr;
    }

    sol::table initMenuPackage(const Context& context)
    {
        sol::state_view lua = context.mLua->sol();
        sol::table api(lua, sol::create);

        api["STATE"]
            = LuaUtil::makeStrictReadOnly(context.mLua->tableFromPairs<std::string_view, MWBase::StateManager::State>({
                { "NoGame", MWBase::StateManager::State_NoGame },
                { "Running", MWBase::StateManager::State_Running },
                { "Ended", MWBase::StateManager::State_Ended },
            }));

        api["getState"] = []() -> int { return MWBase::Environment::get().getStateManager()->getState(); };

        api["newGame"] = []() { MWBase::Environment::get().getStateManager()->requestNewGame(); };

        api["loadGame"] = [](std::string_view dir, std::string_view slotName) {
            const MWState::Character* character = findCharacter(dir);
            const MWState::Slot* slot = findSlot(character, slotName);
            if (!slot)
                throw std::runtime_error("Save game slot not found: " + std::string(dir) + "/" + std::string(slotName));
            MWBase::Environment::get().getStateManager()->requestLoad(slot->mPath);
        };

        api["deleteGame"] = [](std::string_view dir, std::string_view slotName) {
            const MWState::Character* character = findCharacter(dir);
            const MWState::Slot* slot = findSlot(character, slotName);
            if (!slot)
                throw std::runtime_error("Save game slot not found: " + std::string(dir) + "/" + std::string(slotName));
            MWBase::Environment::get().getStateManager()->deleteGame(character, slot);
        };

        api["getCurrentSaveDir"] = []() -> sol::optional<std::string> {
            MWBase::StateManager* manager = MWBase::Environment::get().getStateManager();
            const MWState::Character* character = manager->getCurrentCharacter();
            if (character)
                return character->getPath().filename().string();
            else
                return sol::nullopt;
        };

        api["saveGame"] = [](std::string_view description, sol::optional<std::string_view> slotName) {
            MWBase::StateManager* manager = MWBase::Environment::get().getStateManager();
            const MWState::Character* character = manager->getCurrentCharacter();
            const MWState::Slot* slot = nullptr;
            if (slotName)
                slot = findSlot(character, *slotName);
            manager->saveGame(description, slot);
        };

        auto getSaves = [](sol::state_view lua, const MWState::Character& character) {
            sol::table saves(lua, sol::create);
            for (const MWState::Slot& slot : character)
            {
                sol::table slotInfo(lua, sol::create);
                slotInfo["description"] = slot.mProfile.mDescription;
                slotInfo["playerName"] = slot.mProfile.mPlayerName;
                slotInfo["playerLevel"] = slot.mProfile.mPlayerLevel;
                slotInfo["timePlayed"] = slot.mProfile.mTimePlayed;
                sol::table contentFiles(lua, sol::create);
                for (size_t i = 0; i < slot.mProfile.mContentFiles.size(); ++i)
                    contentFiles[LuaUtil::toLuaIndex(i)] = Misc::StringUtils::lowerCase(slot.mProfile.mContentFiles[i]);

                {
                    auto system_time = std::chrono::system_clock::now()
                        - (std::filesystem::file_time_type::clock::now() - slot.mTimeStamp);
                    slotInfo["creationTime"] = std::chrono::duration<double>(system_time.time_since_epoch()).count();
                }

                slotInfo["contentFiles"] = contentFiles;
                saves[slot.mPath.filename().string()] = slotInfo;
            }
            return saves;
        };

        api["getSaves"] = [getSaves](sol::this_state lua, std::string_view dir) -> sol::table {
            const MWState::Character* character = findCharacter(dir);
            if (!character)
                throw std::runtime_error("Saves not found: " + std::string(dir));
            return getSaves(lua, *character);
        };

        api["getAllSaves"] = [getSaves](sol::this_state lua) -> sol::table {
            sol::table saves(lua, sol::create);
            MWBase::StateManager* manager = MWBase::Environment::get().getStateManager();
            for (auto it = manager->characterBegin(); it != manager->characterEnd(); ++it)
                saves[it->getPath().filename().string()] = getSaves(lua, *it);
            return saves;
        };

        api["quit"] = []() { MWBase::Environment::get().getStateManager()->requestQuit(); };

        return LuaUtil::makeReadOnly(api);
    }
}
