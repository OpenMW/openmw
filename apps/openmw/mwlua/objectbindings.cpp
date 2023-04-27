#include "luabindings.hpp"

#include <components/esm3/loadfact.hpp>
#include <components/esm3/loadnpc.hpp>
#include <components/lua/luastate.hpp>

#include "../mwworld/cellstore.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/containerstore.hpp"
#include "../mwworld/player.hpp"
#include "../mwworld/scene.hpp"

#include "../mwmechanics/creaturestats.hpp"

#include "luaevents.hpp"
#include "luamanagerimp.hpp"
#include "types/types.hpp"

namespace sol
{
    template <>
    struct is_automagical<MWLua::LObject> : std::false_type
    {
    };
    template <>
    struct is_automagical<MWLua::GObject> : std::false_type
    {
    };
    template <>
    struct is_automagical<MWLua::LObjectList> : std::false_type
    {
    };
    template <>
    struct is_automagical<MWLua::GObjectList> : std::false_type
    {
    };
    template <>
    struct is_automagical<MWLua::Inventory<MWLua::LObject>> : std::false_type
    {
    };
    template <>
    struct is_automagical<MWLua::Inventory<MWLua::GObject>> : std::false_type
    {
    };
}

namespace MWLua
{

    namespace
    {
        MWWorld::CellStore* findCell(const sol::object& cellOrName, const osg::Vec3f& pos)
        {
            MWWorld::WorldModel* wm = MWBase::Environment::get().getWorldModel();
            MWWorld::CellStore* cell;
            if (cellOrName.is<GCell>())
                cell = cellOrName.as<const GCell&>().mStore;
            else
            {
                std::string_view name = LuaUtil::cast<std::string_view>(cellOrName);
                if (name.empty())
                    cell = nullptr; // default exterior worldspace
                else
                    cell = &wm->getCell(name);
            }
            return &wm->getCellByPosition(pos, cell);
        }

        void teleportPlayer(MWWorld::CellStore* destCell, const osg::Vec3f& pos, const osg::Vec3f& rot)
        {
            MWBase::World* world = MWBase::Environment::get().getWorld();
            ESM::Position esmPos;
            static_assert(sizeof(esmPos) == sizeof(osg::Vec3f) * 2);
            std::memcpy(esmPos.pos, &pos, sizeof(osg::Vec3f));
            std::memcpy(esmPos.rot, &rot, sizeof(osg::Vec3f));
            MWWorld::Ptr ptr = world->getPlayerPtr();
            ptr.getClass().getCreatureStats(ptr).land(false);
            world->getPlayer().setTeleported(true);
            world->changeToCell(destCell->getCell()->getId(), esmPos, true);
        }

        void teleportNotPlayer(
            const MWWorld::Ptr& ptr, MWWorld::CellStore* destCell, const osg::Vec3f& pos, const osg::Vec3f& rot)
        {
            MWBase::World* world = MWBase::Environment::get().getWorld();
            const MWWorld::Class& cls = ptr.getClass();
            if (cls.isActor())
                cls.getCreatureStats(ptr).land(false);
            MWWorld::Ptr newPtr = world->moveObject(ptr, destCell, pos);
            world->rotateObject(newPtr, rot);
            if (!newPtr.getRefData().isEnabled())
                world->enable(newPtr);
        }

        template <typename ObjT>
        using Cell = std::conditional_t<std::is_same_v<ObjT, LObject>, LCell, GCell>;

        template <class ObjectT>
        void registerObjectList(const std::string& prefix, const Context& context)
        {
            using ListT = ObjectList<ObjectT>;
            sol::state_view& lua = context.mLua->sol();
            sol::usertype<ListT> listT = lua.new_usertype<ListT>(prefix + "ObjectList");
            listT[sol::meta_function::to_string]
                = [](const ListT& list) { return "{" + std::to_string(list.mIds->size()) + " objects}"; };
            listT[sol::meta_function::length] = [](const ListT& list) { return list.mIds->size(); };
            listT[sol::meta_function::index] = [](const ListT& list, size_t index) {
                if (index > 0 && index <= list.mIds->size())
                    return ObjectT((*list.mIds)[index - 1]);
                else
                    throw std::runtime_error("Index out of range");
            };
            listT[sol::meta_function::pairs] = lua["ipairsForArray"].template get<sol::function>();
            listT[sol::meta_function::ipairs] = lua["ipairsForArray"].template get<sol::function>();
        }

        template <class ObjectT>
        void addBasicBindings(sol::usertype<ObjectT>& objectT, const Context& context)
        {
            objectT["id"] = sol::readonly_property([](const ObjectT& o) -> std::string { return o.id().toString(); });
            objectT["isValid"] = [](const ObjectT& o) { return !o.ptrOrNull().isEmpty(); };
            objectT["recordId"] = sol::readonly_property(
                [](const ObjectT& o) -> std::string { return o.ptr().getCellRef().getRefId().serializeText(); });
            objectT["cell"] = sol::readonly_property([](const ObjectT& o) -> sol::optional<Cell<ObjectT>> {
                const MWWorld::Ptr& ptr = o.ptr();
                if (ptr.isInCell())
                    return Cell<ObjectT>{ ptr.getCell() };
                else
                    return sol::nullopt;
            });
            objectT["position"] = sol::readonly_property(
                [](const ObjectT& o) -> osg::Vec3f { return o.ptr().getRefData().getPosition().asVec3(); });
            objectT["rotation"] = sol::readonly_property(
                [](const ObjectT& o) -> osg::Vec3f { return o.ptr().getRefData().getPosition().asRotationVec3(); });

            objectT["type"] = sol::readonly_property(
                [types = getTypeToPackageTable(context.mLua->sol())](
                    const ObjectT& o) mutable { return types[getLiveCellRefType(o.ptr().mRef)]; });

            objectT["count"] = sol::readonly_property([](const ObjectT& o) { return o.ptr().getRefData().getCount(); });
            objectT[sol::meta_function::equal_to] = [](const ObjectT& a, const ObjectT& b) { return a.id() == b.id(); };
            objectT[sol::meta_function::to_string] = &ObjectT::toString;
            objectT["sendEvent"] = [context](const ObjectT& dest, std::string eventName, const sol::object& eventData) {
                context.mLuaEvents->addLocalEvent(
                    { dest.id(), std::move(eventName), LuaUtil::serialize(eventData, context.mSerializer) });
            };
            auto getOwnerRecordId = [](const ObjectT& o) -> sol::optional<std::string> {
                ESM::RefId owner = o.ptr().getCellRef().getOwner();
                if (owner.empty())
                    return sol::nullopt;
                else
                    return owner.serializeText();
            };
            auto setOwnerRecordId = [](const ObjectT& obj, sol::optional<std::string_view> ownerId) {
                if (std::is_same_v<ObjectT, LObject> && !dynamic_cast<const SelfObject*>(&obj))
                    throw std::runtime_error("Local scripts can set an owner only on self");
                const MWWorld::Ptr& ptr = obj.ptr();

                if (!ownerId)
                {
                    ptr.getCellRef().setOwner(ESM::RefId());
                    return;
                }
                ESM::RefId owner = ESM::RefId::deserializeText(*ownerId);
                const MWWorld::ESMStore& store = *MWBase::Environment::get().getESMStore();
                if (!store.get<ESM::NPC>().search(owner))
                    throw std::runtime_error("Invalid owner record id");
                ptr.getCellRef().setOwner(owner);
            };
            objectT["ownerRecordId"] = sol::property(getOwnerRecordId, setOwnerRecordId);

            auto getOwnerFactionId = [](const ObjectT& o) -> sol::optional<std::string> {
                ESM::RefId owner = o.ptr().getCellRef().getFaction();
                if (owner.empty())
                    return sol::nullopt;
                else
                    return owner.serializeText();
            };
            auto setOwnerFactionId = [](const ObjectT& object, sol::optional<std::string> ownerId) {
                ESM::RefId ownerFac;
                if (std::is_same_v<ObjectT, LObject> && !dynamic_cast<const SelfObject*>(&object))
                    throw std::runtime_error("Local scripts can set an owner faction only on self");
                if (!ownerId)
                {
                    object.ptr().getCellRef().setFaction(ESM::RefId());
                    return;
                }
                ownerFac = ESM::RefId::deserializeText(*ownerId);
                const MWWorld::ESMStore& store = *MWBase::Environment::get().getESMStore();
                if (!store.get<ESM::Faction>().search(ownerFac))
                    throw std::runtime_error("Invalid owner faction id");
                object.ptr().getCellRef().setFaction(ownerFac);
            };
            objectT["ownerFactionId"] = sol::property(getOwnerFactionId, setOwnerFactionId);

            auto getOwnerFactionRank = [](const ObjectT& o) -> int { return o.ptr().getCellRef().getFactionRank(); };
            auto setOwnerFactionRank = [](const ObjectT& object, int factionRank) {
                if (std::is_same_v<ObjectT, LObject> && !dynamic_cast<const SelfObject*>(&object))
                    throw std::runtime_error("Local scripts can set an owner faction rank only on self");
                object.ptr().getCellRef().setFactionRank(factionRank);
            };
            objectT["ownerFactionRank"] = sol::property(getOwnerFactionRank, setOwnerFactionRank);

            objectT["activateBy"] = [](const ObjectT& object, const ObjectT& actor) {
                const MWWorld::Ptr& objPtr = object.ptr();
                const MWWorld::Ptr& actorPtr = actor.ptr();
                uint32_t esmRecordType = actorPtr.getType();
                if (esmRecordType != ESM::REC_CREA && esmRecordType != ESM::REC_NPC_)
                    throw std::runtime_error(
                        "The argument of `activateBy` must be an actor who activates the object. Got: "
                        + actor.toString());
                if (objPtr.getRefData().activate())
                    MWBase::Environment::get().getLuaManager()->objectActivated(objPtr, actorPtr);
            };

            auto isEnabled = [](const ObjectT& o) { return o.ptr().getRefData().isEnabled(); };
            auto setEnabled = [context](const GObject& object, bool enable) {
                if (enable && object.ptr().getRefData().isDeleted())
                    throw std::runtime_error("Object is removed");
                context.mLuaManager->addAction([object, enable] {
                    if (object.ptr().isInCell())
                    {
                        if (enable)
                            MWBase::Environment::get().getWorld()->enable(object.ptr());
                        else
                            MWBase::Environment::get().getWorld()->disable(object.ptr());
                    }
                    else
                    {
                        if (enable)
                            object.ptr().getRefData().enable();
                        else
                            throw std::runtime_error("Objects in containers can't be disabled");
                    }
                });
            };
            if constexpr (std::is_same_v<ObjectT, GObject>)
                objectT["enabled"] = sol::property(isEnabled, setEnabled);
            else
                objectT["enabled"] = sol::readonly_property(isEnabled);

            if constexpr (std::is_same_v<ObjectT, GObject>)
            { // Only for global scripts
                objectT["addScript"] = [context](const GObject& object, std::string_view path, sol::object initData) {
                    const LuaUtil::ScriptsConfiguration& cfg = context.mLua->getConfiguration();
                    std::optional<int> scriptId = cfg.findId(path);
                    if (!scriptId)
                        throw std::runtime_error("Unknown script: " + std::string(path));
                    if (!(cfg[*scriptId].mFlags & ESM::LuaScriptCfg::sCustom))
                        throw std::runtime_error(
                            "Script without CUSTOM tag can not be added dynamically: " + std::string(path));
                    if (object.ptr().getType() == ESM::REC_STAT)
                        throw std::runtime_error("Attaching scripts to Static is not allowed: " + std::string(path));
                    if (initData != sol::nil)
                        context.mLuaManager->addCustomLocalScript(object.ptr(), *scriptId,
                            LuaUtil::serialize(LuaUtil::cast<sol::table>(initData), context.mSerializer));
                    else
                        context.mLuaManager->addCustomLocalScript(
                            object.ptr(), *scriptId, cfg[*scriptId].mInitializationData);
                };
                objectT["hasScript"] = [lua = context.mLua](const GObject& object, std::string_view path) {
                    const LuaUtil::ScriptsConfiguration& cfg = lua->getConfiguration();
                    std::optional<int> scriptId = cfg.findId(path);
                    if (!scriptId)
                        return false;
                    MWWorld::Ptr ptr = object.ptr();
                    LocalScripts* localScripts = ptr.getRefData().getLuaScripts();
                    if (localScripts)
                        return localScripts->hasScript(*scriptId);
                    else
                        return false;
                };
                objectT["removeScript"] = [lua = context.mLua](const GObject& object, std::string_view path) {
                    const LuaUtil::ScriptsConfiguration& cfg = lua->getConfiguration();
                    std::optional<int> scriptId = cfg.findId(path);
                    if (!scriptId)
                        throw std::runtime_error("Unknown script: " + std::string(path));
                    MWWorld::Ptr ptr = object.ptr();
                    LocalScripts* localScripts = ptr.getRefData().getLuaScripts();
                    if (!localScripts || !localScripts->hasScript(*scriptId))
                        throw std::runtime_error("There is no script " + std::string(path) + " on " + ptr.toString());
                    if (localScripts->getAutoStartConf().count(*scriptId) > 0)
                        throw std::runtime_error("Autostarted script can not be removed: " + std::string(path));
                    localScripts->removeScript(*scriptId);
                };

                auto removeFn = [context](const GObject& object, int countToRemove) {
                    MWWorld::Ptr ptr = object.ptr();
                    int currentCount = ptr.getRefData().getCount();
                    if (countToRemove <= 0 || countToRemove > currentCount)
                        throw std::runtime_error("Can't remove " + std::to_string(countToRemove) + " of "
                            + std::to_string(currentCount) + " items");
                    ptr.getRefData().setCount(currentCount - countToRemove); // Immediately change count
                    if (ptr.getContainerStore() || currentCount == countToRemove)
                    {
                        // Delayed action to trigger side effects
                        context.mLuaManager->addAction([object, countToRemove] {
                            MWWorld::Ptr ptr = object.ptr();
                            // Restore original count
                            ptr.getRefData().setCount(ptr.getRefData().getCount() + countToRemove);
                            // And now remove properly
                            if (ptr.getContainerStore())
                                ptr.getContainerStore()->remove(ptr, countToRemove);
                            else
                            {
                                MWBase::Environment::get().getWorld()->disable(object.ptr());
                                MWBase::Environment::get().getWorld()->deleteObject(ptr);
                            }
                        });
                    }
                };
                objectT["remove"] = [removeFn](const GObject& object, sol::optional<int> count) {
                    removeFn(object, count.value_or(object.ptr().getRefData().getCount()));
                };
                objectT["split"] = [removeFn](const GObject& object, int count) -> GObject {
                    removeFn(object, count);

                    // Doesn't matter which cell to use because the new instance will be in disabled state.
                    MWWorld::CellStore* cell = MWBase::Environment::get().getWorldScene()->getCurrentCell();

                    const MWWorld::Ptr& ptr = object.ptr();
                    MWWorld::Ptr splitted = ptr.getClass().copyToCell(ptr, *cell, count);
                    splitted.getRefData().disable();
                    return GObject(getId(splitted));
                };
                objectT["moveInto"] = [removeFn, context](const GObject& object, const Inventory<GObject>& inventory) {
                    // Currently moving to or from containers makes a copy and removes the original.
                    // TODO(#6148): actually move rather than copy and preserve RefNum
                    int count = object.ptr().getRefData().getCount();
                    removeFn(object, count);
                    context.mLuaManager->addAction([item = object, count, cont = inventory.mObj] {
                        auto& refData = item.ptr().getRefData();
                        refData.setCount(count); // temporarily undo removal to run ContainerStore::add
                        refData.enable();
                        cont.ptr().getClass().getContainerStore(cont.ptr()).add(item.ptr(), count, false);
                        refData.setCount(0);
                    });
                };
                objectT["teleport"] = [removeFn, context](const GObject& object, const sol::object& cellOrName,
                                          const osg::Vec3f& pos, const sol::optional<osg::Vec3f>& optRot) {
                    MWWorld::CellStore* cell = findCell(cellOrName, pos);
                    MWWorld::Ptr ptr = object.ptr();
                    if (ptr.getRefData().isDeleted())
                        throw std::runtime_error("Object is removed");
                    if (ptr.getContainerStore())
                    {
                        // Currently moving to or from containers makes a copy and removes the original.
                        // TODO(#6148): actually move rather than copy and preserve RefNum
                        MWWorld::Ptr newPtr = ptr.getClass().copyToCell(ptr, *cell, ptr.getRefData().getCount());
                        newPtr.getRefData().disable();
                        removeFn(object, ptr.getRefData().getCount());
                        ptr = newPtr;
                    }
                    osg::Vec3f rot = optRot ? *optRot : ptr.getRefData().getPosition().asRotationVec3();
                    if (ptr == MWBase::Environment::get().getWorld()->getPlayerPtr())
                        context.mLuaManager->addTeleportPlayerAction(
                            [cell, pos, rot] { teleportPlayer(cell, pos, rot); });
                    else
                        context.mLuaManager->addAction(
                            [obj = Object(ptr), cell, pos, rot] { teleportNotPlayer(obj.ptr(), cell, pos, rot); },
                            "TeleportAction");
                };
            }
        }

        template <class ObjectT>
        void addInventoryBindings(sol::usertype<ObjectT>& objectT, const std::string& prefix, const Context& context)
        {
            using InventoryT = Inventory<ObjectT>;
            sol::usertype<InventoryT> inventoryT = context.mLua->sol().new_usertype<InventoryT>(prefix + "Inventory");

            inventoryT[sol::meta_function::to_string]
                = [](const InventoryT& inv) { return "Inventory[" + inv.mObj.toString() + "]"; };

            inventoryT["getAll"] = [ids = getPackageToTypeTable(context.mLua->sol())](
                                       const InventoryT& inventory, sol::optional<sol::table> type) {
                int mask = -1;
                sol::optional<uint32_t> typeId = sol::nullopt;
                if (type.has_value())
                    typeId = ids[*type];
                else
                    mask = MWWorld::ContainerStore::Type_All;

                if (typeId.has_value())
                {
                    switch (*typeId)
                    {
                        case ESM::REC_ALCH:
                            mask = MWWorld::ContainerStore::Type_Potion;
                            break;
                        case ESM::REC_ARMO:
                            mask = MWWorld::ContainerStore::Type_Armor;
                            break;
                        case ESM::REC_BOOK:
                            mask = MWWorld::ContainerStore::Type_Book;
                            break;
                        case ESM::REC_CLOT:
                            mask = MWWorld::ContainerStore::Type_Clothing;
                            break;
                        case ESM::REC_INGR:
                            mask = MWWorld::ContainerStore::Type_Ingredient;
                            break;
                        case ESM::REC_LIGH:
                            mask = MWWorld::ContainerStore::Type_Light;
                            break;
                        case ESM::REC_MISC:
                            mask = MWWorld::ContainerStore::Type_Miscellaneous;
                            break;
                        case ESM::REC_WEAP:
                            mask = MWWorld::ContainerStore::Type_Weapon;
                            break;
                        case ESM::REC_APPA:
                            mask = MWWorld::ContainerStore::Type_Apparatus;
                            break;
                        case ESM::REC_LOCK:
                            mask = MWWorld::ContainerStore::Type_Lockpick;
                            break;
                        case ESM::REC_PROB:
                            mask = MWWorld::ContainerStore::Type_Probe;
                            break;
                        case ESM::REC_REPA:
                            mask = MWWorld::ContainerStore::Type_Repair;
                            break;
                        default:;
                    }
                }

                if (mask == -1)
                    throw std::runtime_error(
                        std::string("Incorrect type argument in inventory:getAll: " + LuaUtil::toString(*type)));

                const MWWorld::Ptr& ptr = inventory.mObj.ptr();
                MWWorld::ContainerStore& store = ptr.getClass().getContainerStore(ptr);
                ObjectIdList list = std::make_shared<std::vector<ObjectId>>();
                auto it = store.begin(mask);
                while (it.getType() != -1)
                {
                    const MWWorld::Ptr& item = *(it++);
                    MWBase::Environment::get().getWorldModel()->registerPtr(item);
                    list->push_back(getId(item));
                }
                return ObjectList<ObjectT>{ list };
            };

            inventoryT["countOf"] = [](const InventoryT& inventory, std::string_view recordId) {
                const MWWorld::Ptr& ptr = inventory.mObj.ptr();
                MWWorld::ContainerStore& store = ptr.getClass().getContainerStore(ptr);
                return store.count(ESM::RefId::stringRefId(recordId));
            };
            inventoryT["find"] = [](const InventoryT& inventory, std::string_view recordId) -> sol::optional<ObjectT> {
                const MWWorld::Ptr& ptr = inventory.mObj.ptr();
                MWWorld::ContainerStore& store = ptr.getClass().getContainerStore(ptr);
                auto itemId = ESM::RefId::stringRefId(recordId);
                for (const MWWorld::Ptr& item : store)
                {
                    if (item.getCellRef().getRefId() == itemId)
                    {
                        MWBase::Environment::get().getWorldModel()->registerPtr(item);
                        return ObjectT(getId(item));
                    }
                }
                return sol::nullopt;
            };
            inventoryT["findAll"] = [](const InventoryT& inventory, std::string_view recordId) {
                const MWWorld::Ptr& ptr = inventory.mObj.ptr();
                MWWorld::ContainerStore& store = ptr.getClass().getContainerStore(ptr);
                auto itemId = ESM::RefId::stringRefId(recordId);
                ObjectIdList list = std::make_shared<std::vector<ObjectId>>();
                for (const MWWorld::Ptr& item : store)
                {
                    if (item.getCellRef().getRefId() == itemId)
                    {
                        MWBase::Environment::get().getWorldModel()->registerPtr(item);
                        list->push_back(getId(item));
                    }
                }
                return ObjectList<ObjectT>{ list };
            };
        }

        template <class ObjectT>
        void initObjectBindings(const std::string& prefix, const Context& context)
        {
            sol::usertype<ObjectT> objectT
                = context.mLua->sol().new_usertype<ObjectT>(prefix + "Object", sol::base_classes, sol::bases<Object>());
            addBasicBindings<ObjectT>(objectT, context);
            addInventoryBindings<ObjectT>(objectT, prefix, context);

            registerObjectList<ObjectT>(prefix, context);
        }
    } // namespace

    void initObjectBindingsForLocalScripts(const Context& context)
    {
        initObjectBindings<LObject>("L", context);
    }

    void initObjectBindingsForGlobalScripts(const Context& context)
    {
        initObjectBindings<GObject>("G", context);
    }

}
