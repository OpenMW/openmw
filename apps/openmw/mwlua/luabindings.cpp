#include "luabindings.hpp"

#include <chrono>

#include <components/esm/attr.hpp>
#include <components/esm3/loadacti.hpp>
#include <components/esm3/loadalch.hpp>
#include <components/esm3/loadarmo.hpp>
#include <components/esm3/loadbook.hpp>
#include <components/esm3/loadclot.hpp>
#include <components/esm3/loadmisc.hpp>
#include <components/esm3/loadskil.hpp>
#include <components/esm3/loadweap.hpp>
#include <components/lua/l10n.hpp>
#include <components/lua/luastate.hpp>
#include <components/lua/utilpackage.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/statemanager.hpp"
#include "../mwworld/action.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/manualref.hpp"
#include "../mwworld/scene.hpp"
#include "../mwworld/store.hpp"

#include "luaevents.hpp"
#include "luamanagerimp.hpp"
#include "worldview.hpp"

#include "camerabindings.hpp"
#include "cellbindings.hpp"
#include "debugbindings.hpp"
#include "inputbindings.hpp"
#include "magicbindings.hpp"
#include "nearbybindings.hpp"
#include "objectbindings.hpp"
#include "postprocessingbindings.hpp"
#include "types/types.hpp"
#include "uibindings.hpp"

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

    static void addTimeBindings(sol::table& api, const Context& context, bool global)
    {
        MWBase::World* world = MWBase::Environment::get().getWorld();

        api["getSimulationTime"] = [world = context.mWorldView]() { return world->getSimulationTime(); };
        api["getSimulationTimeScale"] = [world]() { return world->getSimulationTimeScale(); };
        api["getGameTime"] = [world = context.mWorldView]() { return world->getGameTime(); };
        api["getGameTimeScale"] = [world = context.mWorldView]() { return world->getGameTimeScale(); };
        api["isWorldPaused"] = [world = context.mWorldView]() { return world->isPaused(); };
        api["getRealTime"] = []() {
            return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
        };

        if (!global)
            return;

        api["setGameTimeScale"] = [world = context.mWorldView](double scale) { world->setGameTimeScale(scale); };

        api["setSimulationTimeScale"] = [context, world](float scale) {
            context.mLuaManager->addAction([scale, world] { world->setSimulationTimeScale(scale); });
        };

        // TODO: Ability to pause/resume world from Lua (needed for UI dehardcoding)
        // api["pause"] = []() {};
        // api["resume"] = []() {};
    }

    static sol::table initCorePackage(const Context& context)
    {
        auto* lua = context.mLua;
        sol::table api(lua->sol(), sol::create);
        api["API_REVISION"] = 39;
        api["quit"] = [lua]() {
            Log(Debug::Warning) << "Quit requested by a Lua script.\n" << lua->debugTraceback();
            MWBase::Environment::get().getStateManager()->requestQuit();
        };
        api["sendGlobalEvent"] = [context](std::string eventName, const sol::object& eventData) {
            context.mLuaEvents->addGlobalEvent(
                { std::move(eventName), LuaUtil::serialize(eventData, context.mSerializer) });
        };
        api["getContentList"] = [](sol::this_state lua) -> sol::table {
            const std::vector<std::string>& contentList = MWBase::Environment::get().getWorld()->getContentFiles();
            sol::table res(lua, sol::create);
            int i = 1;
            for (const std::string& s : contentList)
                res[i++] = Misc::StringUtils::lowerCase(s);
            return res;
        };
        api["getContentFileIndex"] = [](std::string_view contentFile) -> sol::optional<int> {
            const std::vector<std::string>& contentList = MWBase::Environment::get().getWorld()->getContentFiles();
            for (size_t i = 0; i < contentList.size(); ++i)
                if (Misc::StringUtils::ciEqual(contentList[i], contentFile))
                    return i + 1;
            return sol::nullopt;
        };
        api["getFormId"] = [](std::string_view contentFile, unsigned int index) -> std::string {
            const std::vector<std::string>& contentList = MWBase::Environment::get().getWorld()->getContentFiles();
            for (size_t i = 0; i < contentList.size(); ++i)
                if (Misc::StringUtils::ciEqual(contentList[i], contentFile))
                    return ESM::RefId(ESM::FormIdRefId(ESM::FormId{ index, int(i) })).serializeText();
            throw std::runtime_error("Content file not found: " + std::string(contentFile));
        };
        addTimeBindings(api, context, false);
        api["magic"] = initCoreMagicBindings(context);
        api["l10n"] = LuaUtil::initL10nLoader(lua->sol(), MWBase::Environment::get().getL10nManager());
        const MWWorld::Store<ESM::GameSetting>* gmst
            = &MWBase::Environment::get().getESMStore()->get<ESM::GameSetting>();
        api["getGMST"] = [lua = context.mLua, gmst](const std::string& setting) -> sol::object {
            const ESM::Variant& value = gmst->find(setting)->mValue;
            if (value.getType() == ESM::VT_String)
                return sol::make_object<std::string>(lua->sol(), value.getString());
            else if (value.getType() == ESM::VT_Int)
                return sol::make_object<int>(lua->sol(), value.getInteger());
            else
                return sol::make_object<float>(lua->sol(), value.getFloat());
        };

        sol::table skill(context.mLua->sol(), sol::create);
        api["SKILL"] = LuaUtil::makeStrictReadOnly(skill);
        for (int id = 0; id < ESM::Skill::Length; ++id)
            skill[ESM::Skill::sSkillNames[id]] = Misc::StringUtils::lowerCase(ESM::Skill::sSkillNames[id]);

        sol::table attribute(context.mLua->sol(), sol::create);
        api["ATTRIBUTE"] = LuaUtil::makeStrictReadOnly(attribute);
        for (int id = 0; id < ESM::Attribute::Length; ++id)
            attribute[ESM::Attribute::sAttributeNames[id]]
                = Misc::StringUtils::lowerCase(ESM::Attribute::sAttributeNames[id]);

        return LuaUtil::makeReadOnly(api);
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
        cells[sol::meta_function::index] = [cells3Store, cells4Store](const CellsStore&, size_t index) -> GCell {
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

    static sol::table initWorldPackage(const Context& context)
    {
        sol::table api(context.mLua->sol(), sol::create);
        WorldView* worldView = context.mWorldView;
        addTimeBindings(api, context, true);
        addCellGetters(api, context);
        api["activeActors"] = GObjectList{ worldView->getActorsInScene() };
        api["createObject"] = [](std::string_view recordId, sol::optional<int> count) -> GObject {
            // Doesn't matter which cell to use because the new object will be in disabled state.
            MWWorld::CellStore* cell = MWBase::Environment::get().getWorldScene()->getCurrentCell();

            MWWorld::ManualRef mref(*MWBase::Environment::get().getESMStore(), ESM::RefId::deserializeText(recordId));
            const MWWorld::Ptr& ptr = mref.getPtr();
            ptr.getRefData().disable();
            MWWorld::Ptr newPtr = ptr.getClass().copyToCell(ptr, *cell, count.value_or(1));
            return GObject(newPtr);
        };
        api["getObjectByFormId"] = [](std::string_view formIdStr) -> GObject {
            ESM::RefId refId = ESM::RefId::deserializeText(formIdStr);
            if (!refId.is<ESM::FormIdRefId>())
                throw std::runtime_error("FormId expected, got " + std::string(formIdStr) + "; use core.getFormId");
            return GObject(refId.getIf<ESM::FormIdRefId>()->getValue());
        };

        // Creates a new record in the world database.
        api["createRecord"] = sol::overload(
            [](const ESM::Activator& activator) -> const ESM::Activator* {
                return MWBase::Environment::get().getESMStore()->insert(activator);
            },
            [](const ESM::Armor& armor) -> const ESM::Armor* {
                return MWBase::Environment::get().getESMStore()->insert(armor);
            },
            [](const ESM::Clothing& clothing) -> const ESM::Clothing* {
                return MWBase::Environment::get().getESMStore()->insert(clothing);
            },
            [](const ESM::Book& book) -> const ESM::Book* {
                return MWBase::Environment::get().getESMStore()->insert(book);
            },
            [](const ESM::Miscellaneous& misc) -> const ESM::Miscellaneous* {
                return MWBase::Environment::get().getESMStore()->insert(misc);
            },
            [](const ESM::Potion& potion) -> const ESM::Potion* {
                return MWBase::Environment::get().getESMStore()->insert(potion);
            },
            [](const ESM::Weapon& weapon) -> const ESM::Weapon* {
                return MWBase::Environment::get().getESMStore()->insert(weapon);
            });

        api["_runStandardActivationAction"] = [context](const GObject& object, const GObject& actor) {
            context.mLuaManager->addAction(
                [object, actor] {
                    const MWWorld::Ptr& objPtr = object.ptr();
                    const MWWorld::Ptr& actorPtr = actor.ptr();
                    objPtr.getClass().activate(objPtr, actorPtr)->execute(actorPtr);
                },
                "_runStandardActivationAction");
        };

        return LuaUtil::makeReadOnly(api);
    }

    std::map<std::string, sol::object> initCommonPackages(const Context& context)
    {
        sol::state_view lua = context.mLua->sol();
        WorldView* w = context.mWorldView;
        return {
            { "openmw.async",
                LuaUtil::getAsyncPackageInitializer(
                    lua, [w] { return w->getSimulationTime(); }, [w] { return w->getGameTime(); }) },
            { "openmw.core", initCorePackage(context) },
            { "openmw.types", initTypesPackage(context) },
            { "openmw.util", LuaUtil::initUtilPackage(lua) },
        };
    }

    std::map<std::string, sol::object> initGlobalPackages(const Context& context)
    {
        initObjectBindingsForGlobalScripts(context);
        initCellBindingsForGlobalScripts(context);
        return {
            { "openmw.world", initWorldPackage(context) },
        };
    }

    std::map<std::string, sol::object> initLocalPackages(const Context& context)
    {
        initObjectBindingsForLocalScripts(context);
        initCellBindingsForLocalScripts(context);
        LocalScripts::initializeSelfPackage(context);
        return {
            { "openmw.nearby", initNearbyPackage(context) },
        };
    }

    std::map<std::string, sol::object> initPlayerPackages(const Context& context)
    {
        return {
            { "openmw.camera", initCameraPackage(context.mLua->sol()) },
            { "openmw.debug", initDebugPackage(context) },
            { "openmw.input", initInputPackage(context) },
            { "openmw.postprocessing", initPostprocessingPackage(context) },
            { "openmw.ui", initUserInterfacePackage(context) },
        };
    }

}
