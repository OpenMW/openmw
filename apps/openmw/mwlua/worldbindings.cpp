#include "worldbindings.hpp"

#include <components/esm3/loadacti.hpp>
#include <components/esm3/loadalch.hpp>
#include <components/esm3/loadarmo.hpp>
#include <components/esm3/loadbook.hpp>
#include <components/esm3/loadclot.hpp>
#include <components/esm3/loadmisc.hpp>
#include <components/esm3/loadskil.hpp>
#include <components/esm3/loadweap.hpp>
#include <components/lua/luastate.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/statemanager.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"
#include "../mwworld/action.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/datetimemanager.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/manualref.hpp"
#include "../mwworld/store.hpp"
#include "../mwworld/worldmodel.hpp"

#include "luamanagerimp.hpp"

#include "corebindings.hpp"
#include "mwscriptbindings.hpp"

namespace MWLua
{
    struct CellsStore
    {
    };
}

namespace sol
{
    template <>
    struct is_automagical<MWLua::CellsStore> : std::false_type
    {
    };
}

namespace MWLua
{

    static void checkGameInitialized(LuaUtil::LuaState* lua)
    {
        if (MWBase::Environment::get().getStateManager()->getState() == MWBase::StateManager::State_NoGame)
            throw std::runtime_error(
                "This function cannot be used until the game is fully initialized.\n" + lua->debugTraceback());
    }

    static void addWorldTimeBindings(sol::table& api, const Context& context)
    {
        MWWorld::DateTimeManager* timeManager = MWBase::Environment::get().getWorld()->getTimeManager();

        api["setGameTimeScale"] = [timeManager](double scale) { timeManager->setGameTimeScale(scale); };
        api["setSimulationTimeScale"] = [context, timeManager](float scale) {
            context.mLuaManager->addAction([scale, timeManager] { timeManager->setSimulationTimeScale(scale); });
        };

        api["pause"]
            = [timeManager](sol::optional<std::string_view> tag) { timeManager->pause(tag.value_or("paused")); };
        api["unpause"]
            = [timeManager](sol::optional<std::string_view> tag) { timeManager->unpause(tag.value_or("paused")); };
        api["getPausedTags"] = [timeManager](sol::this_state lua) {
            sol::table res(lua, sol::create);
            for (const std::string& tag : timeManager->getPausedTags())
                res[tag] = tag;
            return res;
        };
    }

    static void addCellGetters(sol::table& api, const Context& context)
    {
        api["getCellByName"] = [](std::string_view name) {
            return GCell{ &MWBase::Environment::get().getWorldModel()->getCell(name, /*forceLoad=*/false) };
        };
        api["getExteriorCell"] = [](int x, int y, sol::object cellOrName) {
            ESM::RefId worldspace;
            if (cellOrName.is<GCell>())
                worldspace = cellOrName.as<GCell>().mStore->getCell()->getWorldSpace();
            else if (cellOrName.is<std::string_view>() && !cellOrName.as<std::string_view>().empty())
                worldspace = MWBase::Environment::get()
                                 .getWorldModel()
                                 ->getCell(cellOrName.as<std::string_view>())
                                 .getCell()
                                 ->getWorldSpace();
            else
                worldspace = ESM::Cell::sDefaultWorldspaceId;
            return GCell{ &MWBase::Environment::get().getWorldModel()->getExterior(
                ESM::ExteriorCellLocation(x, y, worldspace), /*forceLoad=*/false) };
        };

        const MWWorld::Store<ESM::Cell>* cells3Store = &MWBase::Environment::get().getESMStore()->get<ESM::Cell>();
        const MWWorld::Store<ESM4::Cell>* cells4Store = &MWBase::Environment::get().getESMStore()->get<ESM4::Cell>();
        sol::usertype<CellsStore> cells = context.mLua->sol().new_usertype<CellsStore>("Cells");
        cells[sol::meta_function::length]
            = [cells3Store, cells4Store](const CellsStore&) { return cells3Store->getSize() + cells4Store->getSize(); };
        cells[sol::meta_function::index]
            = [cells3Store, cells4Store](const CellsStore&, size_t index) -> sol::optional<GCell> {
            if (index > cells3Store->getSize() + cells3Store->getSize() || index == 0)
                return sol::nullopt;

            index--; // Translate from Lua's 1-based indexing.
            if (index < cells3Store->getSize())
            {
                const ESM::Cell* cellRecord = cells3Store->at(index);
                return GCell{ &MWBase::Environment::get().getWorldModel()->getCell(
                    cellRecord->mId, /*forceLoad=*/false) };
            }
            else
            {
                const ESM4::Cell* cellRecord = cells4Store->at(index - cells3Store->getSize());
                return GCell{ &MWBase::Environment::get().getWorldModel()->getCell(
                    cellRecord->mId, /*forceLoad=*/false) };
            }
        };
        cells[sol::meta_function::pairs] = context.mLua->sol()["ipairsForArray"].template get<sol::function>();
        cells[sol::meta_function::ipairs] = context.mLua->sol()["ipairsForArray"].template get<sol::function>();
        api["cells"] = CellsStore{};
    }

    sol::table initWorldPackage(const Context& context)
    {
        sol::table api(context.mLua->sol(), sol::create);

        addCoreTimeBindings(api, context);
        addWorldTimeBindings(api, context);
        addCellGetters(api, context);
        api["mwscript"] = initMWScriptBindings(context);

        ObjectLists* objectLists = context.mObjectLists;
        api["activeActors"] = GObjectList{ objectLists->getActorsInScene() };
        api["players"] = GObjectList{ objectLists->getPlayers() };

        api["createObject"] = [lua = context.mLua](std::string_view recordId, sol::optional<int> count) -> GObject {
            checkGameInitialized(lua);
            MWWorld::ManualRef mref(*MWBase::Environment::get().getESMStore(), ESM::RefId::deserializeText(recordId));
            const MWWorld::Ptr& ptr = mref.getPtr();
            ptr.getRefData().disable();
            MWWorld::CellStore& cell = MWBase::Environment::get().getWorldModel()->getDraftCell();
            MWWorld::Ptr newPtr = ptr.getClass().copyToCell(ptr, cell, count.value_or(1));
            return GObject(newPtr);
        };
        api["getObjectByFormId"] = [](std::string_view formIdStr) -> GObject {
            ESM::RefId refId = ESM::RefId::deserializeText(formIdStr);
            if (!refId.is<ESM::FormId>())
                throw std::runtime_error("FormId expected, got " + std::string(formIdStr) + "; use core.getFormId");
            return GObject(*refId.getIf<ESM::FormId>());
        };

        // Creates a new record in the world database.
        api["createRecord"] = sol::overload(
            [lua = context.mLua](const ESM::Activator& activator) -> const ESM::Activator* {
                checkGameInitialized(lua);
                return MWBase::Environment::get().getESMStore()->insert(activator);
            },
            [lua = context.mLua](const ESM::Armor& armor) -> const ESM::Armor* {
                checkGameInitialized(lua);
                return MWBase::Environment::get().getESMStore()->insert(armor);
            },
            [lua = context.mLua](const ESM::Clothing& clothing) -> const ESM::Clothing* {
                checkGameInitialized(lua);
                return MWBase::Environment::get().getESMStore()->insert(clothing);
            },
            [lua = context.mLua](const ESM::Book& book) -> const ESM::Book* {
                checkGameInitialized(lua);
                return MWBase::Environment::get().getESMStore()->insert(book);
            },
            [lua = context.mLua](const ESM::Miscellaneous& misc) -> const ESM::Miscellaneous* {
                checkGameInitialized(lua);
                return MWBase::Environment::get().getESMStore()->insert(misc);
            },
            [lua = context.mLua](const ESM::Potion& potion) -> const ESM::Potion* {
                checkGameInitialized(lua);
                return MWBase::Environment::get().getESMStore()->insert(potion);
            },
            [lua = context.mLua](const ESM::Weapon& weapon) -> const ESM::Weapon* {
                checkGameInitialized(lua);
                return MWBase::Environment::get().getESMStore()->insert(weapon);
            });

        api["_runStandardActivationAction"] = [context](const GObject& object, const GObject& actor) {
            if (!object.ptr().getRefData().activate())
                return;
            context.mLuaManager->addAction(
                [object, actor] {
                    const MWWorld::Ptr& objPtr = object.ptr();
                    const MWWorld::Ptr& actorPtr = actor.ptr();
                    objPtr.getClass().activate(objPtr, actorPtr)->execute(actorPtr);
                },
                "_runStandardActivationAction");
        };
        api["_runStandardUseAction"] = [context](const GObject& object, const GObject& actor, bool force) {
            context.mLuaManager->addAction(
                [object, actor, force] {
                    const MWWorld::Ptr& actorPtr = actor.ptr();
                    const MWWorld::Ptr& objectPtr = object.ptr();
                    if (actorPtr == MWBase::Environment::get().getWorld()->getPlayerPtr())
                        MWBase::Environment::get().getWindowManager()->useItem(objectPtr, force);
                    else
                    {
                        std::unique_ptr<MWWorld::Action> action = objectPtr.getClass().use(objectPtr, force);
                        action->execute(actorPtr, true);
                    }
                },
                "_runStandardUseAction");
        };

        return LuaUtil::makeReadOnly(api);
    }
}
