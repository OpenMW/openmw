#include "luabindings.hpp"

#include <components/lua/luastate.hpp>

#include "../mwworld/class.hpp"
#include "../mwworld/containerstore.hpp"
#include "../mwworld/inventorystore.hpp"

#include "eventqueue.hpp"
#include "luamanagerimp.hpp"
#include "types/types.hpp"

namespace sol
{
    template <>
    struct is_automagical<MWLua::LObject> : std::false_type {};
    template <>
    struct is_automagical<MWLua::GObject> : std::false_type {};
    template <>
    struct is_automagical<MWLua::LObjectList> : std::false_type {};
    template <>
    struct is_automagical<MWLua::GObjectList> : std::false_type {};
    template <>
    struct is_automagical<MWLua::Inventory<MWLua::LObject>> : std::false_type {};
    template <>
    struct is_automagical<MWLua::Inventory<MWLua::GObject>> : std::false_type {};
}

namespace MWLua
{

    namespace {

        template <typename ObjT>
        using Cell = std::conditional_t<std::is_same_v<ObjT, LObject>, LCell, GCell>;

        template <class ObjectT>
        void registerObjectList(const std::string& prefix, const Context& context)
        {
            using ListT = ObjectList<ObjectT>;
            sol::state& lua = context.mLua->sol();
            ObjectRegistry* registry = context.mWorldView->getObjectRegistry();
            sol::usertype<ListT> listT = lua.new_usertype<ListT>(prefix + "ObjectList");
            listT[sol::meta_function::to_string] =
                [](const ListT& list) { return "{" + std::to_string(list.mIds->size()) + " objects}"; };
            listT[sol::meta_function::length] = [](const ListT& list) { return list.mIds->size(); };
            listT[sol::meta_function::index] = [registry](const ListT& list, size_t index)
            {
                if (index > 0 && index <= list.mIds->size())
                    return ObjectT((*list.mIds)[index - 1], registry);
                else
                    throw std::runtime_error("Index out of range");
            };
            if constexpr (std::is_same_v<ObjectT, GObject>)
            {
                // GObject and LObject iterators are in separate branches because if they share source code
                // there is a collision in sol and only one iterator can be mapped to Lua.
                auto iter = sol::make_object(lua, [registry](const GObjectList& l, int64_t i) -> sol::optional<std::tuple<int64_t, GObject>>
                {
                    if (i >= 0 && i < static_cast<int64_t>(l.mIds->size()))
                        return std::make_tuple(i + 1, GObject((*l.mIds)[i], registry));
                    else
                        return sol::nullopt;
                });
                listT[sol::meta_function::ipairs] = [iter](const GObjectList& list) { return std::make_tuple(iter, list, 0); };
            }
            else
            {
                auto iter = sol::make_object(lua, [registry](const LObjectList& l, int64_t i) -> sol::optional<std::tuple<int64_t, LObject>>
                {
                    if (i >= 0 && i < static_cast<int64_t>(l.mIds->size()))
                        return std::make_tuple(i + 1, LObject((*l.mIds)[i], registry));
                    else
                        return sol::nullopt;
                });
                listT[sol::meta_function::ipairs] = [iter](const LObjectList& list) { return std::make_tuple(iter, list, 0); };
            }
        }

        template <class ObjectT>
        void addBasicBindings(sol::usertype<ObjectT>& objectT, const Context& context)
        {
            objectT["isValid"] = [](const ObjectT& o) { return o.isValid(); };
            objectT["recordId"] = sol::readonly_property([](const ObjectT& o) -> std::string
            {
                return o.ptr().getCellRef().getRefId();
            });
            objectT["cell"] = sol::readonly_property([](const ObjectT& o) -> sol::optional<Cell<ObjectT>>
            {
                const MWWorld::Ptr& ptr = o.ptr();
                if (ptr.isInCell())
                    return Cell<ObjectT>{ptr.getCell()};
                else
                    return sol::nullopt;
            });
            objectT["position"] = sol::readonly_property([](const ObjectT& o) -> osg::Vec3f
            {
                return o.ptr().getRefData().getPosition().asVec3();
            });
            objectT["rotation"] = sol::readonly_property([](const ObjectT& o) -> osg::Vec3f
            {
                return o.ptr().getRefData().getPosition().asRotationVec3();
            });

            objectT["type"] = sol::readonly_property([types=getTypeToPackageTable(context.mLua->sol())](const ObjectT& o) mutable
            {
                return types[o.ptr().getLuaType()];
            });

            objectT["count"] = sol::readonly_property([](const ObjectT& o) { return o.ptr().getRefData().getCount(); });
            objectT[sol::meta_function::equal_to] = [](const ObjectT& a, const ObjectT& b) { return a.id() == b.id(); };
            objectT[sol::meta_function::to_string] = &ObjectT::toString;
            objectT["sendEvent"] = [context](const ObjectT& dest, std::string eventName, const sol::object& eventData)
            {
                context.mLocalEventQueue->push_back({dest.id(), std::move(eventName), LuaUtil::serialize(eventData, context.mSerializer)});
            };

            objectT["activateBy"] = [context](const ObjectT& o, const ObjectT& actor)
            {
                uint32_t esmRecordType = actor.ptr().getType();
                if (esmRecordType != ESM::REC_CREA && esmRecordType != ESM::REC_NPC_)
                    throw std::runtime_error("The argument of `activateBy` must be an actor who activates the object. Got: " +
                                             ptrToString(actor.ptr()));
                context.mLuaManager->addAction(std::make_unique<ActivateAction>(context.mLua, o.id(), actor.id()));
            };

            if constexpr (std::is_same_v<ObjectT, GObject>)
            {  // Only for global scripts
                objectT["addScript"] = [lua=context.mLua, luaManager=context.mLuaManager](const GObject& object, std::string_view path)
                {
                    const LuaUtil::ScriptsConfiguration& cfg = lua->getConfiguration();
                    std::optional<int> scriptId = cfg.findId(path);
                    if (!scriptId)
                        throw std::runtime_error("Unknown script: " + std::string(path));
                    if (!(cfg[*scriptId].mFlags & ESM::LuaScriptCfg::sCustom))
                        throw std::runtime_error("Script without CUSTOM tag can not be added dynamically: " + std::string(path));
                    if (object.ptr().getType() == ESM::REC_STAT)
                        throw std::runtime_error("Attaching scripts to Static is not allowed: " + std::string(path));
                    luaManager->addCustomLocalScript(object.ptr(), *scriptId);
                };
                objectT["hasScript"] = [lua=context.mLua](const GObject& object, std::string_view path)
                {
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
                objectT["removeScript"] = [lua=context.mLua](const GObject& object, std::string_view path)
                {
                    const LuaUtil::ScriptsConfiguration& cfg = lua->getConfiguration();
                    std::optional<int> scriptId = cfg.findId(path);
                    if (!scriptId)
                        throw std::runtime_error("Unknown script: " + std::string(path));
                    MWWorld::Ptr ptr = object.ptr();
                    LocalScripts* localScripts = ptr.getRefData().getLuaScripts();
                    if (!localScripts || !localScripts->hasScript(*scriptId))
                        throw std::runtime_error("There is no script " + std::string(path) + " on " + ptrToString(ptr));
                    ESM::LuaScriptCfg::Flags flags = cfg[*scriptId].mFlags;
                    if ((flags & (localScripts->getAutoStartMode() | ESM::LuaScriptCfg::sCustom)) != ESM::LuaScriptCfg::sCustom)
                        throw std::runtime_error("Autostarted script can not be removed: " + std::string(path));
                    localScripts->removeScript(*scriptId);
                };

                objectT["teleport"] = [context](const GObject& object, std::string_view cell,
                                                const osg::Vec3f& pos, const sol::optional<osg::Vec3f>& optRot)
                {
                    MWWorld::Ptr ptr = object.ptr();
                    osg::Vec3f rot = optRot ? *optRot : ptr.getRefData().getPosition().asRotationVec3();
                    auto action = std::make_unique<TeleportAction>(context.mLua, object.id(), std::string(cell), pos, rot);
                    if (ptr == MWBase::Environment::get().getWorld()->getPlayerPtr())
                        context.mLuaManager->addTeleportPlayerAction(std::move(action));
                    else
                        context.mLuaManager->addAction(std::move(action));
                };
            }
        }

        template <class ObjectT>
        void addInventoryBindings(sol::usertype<ObjectT>& objectT, const std::string& prefix, const Context& context)
        {
            using InventoryT = Inventory<ObjectT>;
            sol::usertype<InventoryT> inventoryT = context.mLua->sol().new_usertype<InventoryT>(prefix + "Inventory");

            inventoryT[sol::meta_function::to_string] =
                [](const InventoryT& inv) { return "Inventory[" + inv.mObj.toString() + "]"; };

            inventoryT["getAll"] = [worldView=context.mWorldView, ids=getPackageToTypeTable(context.mLua->sol())](
                const InventoryT& inventory, sol::optional<sol::table> type)
            {
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
                        case ESM::REC_ALCH: mask = MWWorld::ContainerStore::Type_Potion; break;
                        case ESM::REC_ARMO: mask = MWWorld::ContainerStore::Type_Armor; break;
                        case ESM::REC_BOOK: mask = MWWorld::ContainerStore::Type_Book; break;
                        case ESM::REC_CLOT: mask = MWWorld::ContainerStore::Type_Clothing; break;
                        case ESM::REC_INGR: mask = MWWorld::ContainerStore::Type_Ingredient; break;
                        case ESM::REC_LIGH: mask = MWWorld::ContainerStore::Type_Light; break;
                        case ESM::REC_MISC: mask = MWWorld::ContainerStore::Type_Miscellaneous; break;
                        case ESM::REC_WEAP: mask = MWWorld::ContainerStore::Type_Weapon; break;
                        case ESM::REC_APPA: mask = MWWorld::ContainerStore::Type_Apparatus; break;
                        case ESM::REC_LOCK: mask = MWWorld::ContainerStore::Type_Lockpick; break;
                        case ESM::REC_PROB: mask = MWWorld::ContainerStore::Type_Probe; break;
                        case ESM::REC_REPA: mask = MWWorld::ContainerStore::Type_Repair; break;
                        default:;
                    }
                }

                if (mask == -1)
                    throw std::runtime_error(std::string("Incorrect type argument in inventory:getAll: " + LuaUtil::toString(*type)));

                const MWWorld::Ptr& ptr = inventory.mObj.ptr();
                MWWorld::ContainerStore& store = ptr.getClass().getContainerStore(ptr);
                ObjectIdList list = std::make_shared<std::vector<ObjectId>>();
                auto it = store.begin(mask);
                while (it.getType() != -1)
                {
                    const MWWorld::Ptr& item = *(it++);
                    worldView->getObjectRegistry()->registerPtr(item);
                    list->push_back(getId(item));
                }
                return ObjectList<ObjectT>{list};
            };

            inventoryT["countOf"] = [](const InventoryT& inventory, const std::string& recordId)
            {
                const MWWorld::Ptr& ptr = inventory.mObj.ptr();
                MWWorld::ContainerStore& store = ptr.getClass().getContainerStore(ptr);
                return store.count(recordId);
            };

            if constexpr (std::is_same_v<ObjectT, GObject>)
            {  // Only for global scripts
                // TODO
                // obj.inventory:drop(obj2, [count])
                // obj.inventory:drop(recordId, [count])
                // obj.inventory:addNew(recordId, [count])
                // obj.inventory:remove(obj/recordId, [count])
                /*objectT["moveInto"] = [](const GObject& obj, const InventoryT& inventory) {};
                inventoryT["drop"] = [](const InventoryT& inventory) {};
                inventoryT["addNew"] = [](const InventoryT& inventory) {};
                inventoryT["remove"] = [](const InventoryT& inventory) {};*/
            }
        }

        template <class ObjectT>
        void initObjectBindings(const std::string& prefix, const Context& context)
        {
            sol::usertype<ObjectT> objectT = context.mLua->sol().new_usertype<ObjectT>(
                prefix + "Object", sol::base_classes, sol::bases<Object>());
            addBasicBindings<ObjectT>(objectT, context);
            addInventoryBindings<ObjectT>(objectT, prefix, context);

            registerObjectList<ObjectT>(prefix, context);
        }
    }  // namespace

    void initObjectBindingsForLocalScripts(const Context& context)
    {
        initObjectBindings<LObject>("L", context);
    }

    void initObjectBindingsForGlobalScripts(const Context& context)
    {
        initObjectBindings<GObject>("G", context);
    }

}
