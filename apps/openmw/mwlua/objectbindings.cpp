#include "luabindings.hpp"

#include <components/lua/luastate.hpp>
#include <components/queries/query.hpp>

#include "../mwclass/door.hpp"

#include "../mwworld/containerstore.hpp"
#include "../mwworld/inventorystore.hpp"

#include "eventqueue.hpp"
#include "luamanagerimp.hpp"

namespace MWLua
{
    template <typename ObjectT>
    struct Inventory
    {
        ObjectT mObj;
    };
}

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

    template <class Class>
    static const MWWorld::Ptr& requireClass(const MWWorld::Ptr& ptr)
    {
        if (typeid(Class) != typeid(ptr.getClass()))
        {
            std::string msg = "Requires type '";
            msg.append(getMWClassName(typeid(Class)));
            msg.append("', but applied to ");
            msg.append(ptrToString(ptr));
            throw std::runtime_error(msg);
        }
        return ptr;
    }

    template <class ObjectT>
    static void registerObjectList(const std::string& prefix, const Context& context)
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
        listT["ipairs"] = [registry](const ListT& list)
        {
            auto iter = [registry](const ListT& l, int64_t i) -> sol::optional<std::tuple<int64_t, ObjectT>>
            {
                if (i >= 0 && i < static_cast<int64_t>(l.mIds->size()))
                    return std::make_tuple(i + 1, ObjectT((*l.mIds)[i], registry));
                else
                    return sol::nullopt;
            };
            return std::make_tuple(iter, list, 0);
        };
        listT["select"] = [context](const ListT& list, const Queries::Query& query)
        {
            return ListT{selectObjectsFromList(query, list.mIds, context)};
        };
    }

    template <class ObjectT>
    static void addBasicBindings(sol::usertype<ObjectT>& objectT, const Context& context)
    {
        objectT["isValid"] = [](const ObjectT& o) { return o.isValid(); };
        objectT["recordId"] = sol::readonly_property([](const ObjectT& o) -> std::string
        {
            return o.ptr().getCellRef().getRefId();
        });
        objectT["position"] = sol::readonly_property([](const ObjectT& o) -> osg::Vec3f
        {
            return o.ptr().getRefData().getPosition().asVec3();
        });
        objectT["rotation"] = sol::readonly_property([](const ObjectT& o) -> osg::Vec3f
        {
            return o.ptr().getRefData().getPosition().asRotationVec3();
        });
        objectT["type"] = sol::readonly_property(&ObjectT::type);
        objectT["count"] = sol::readonly_property([](const ObjectT& o) { return o.ptr().getRefData().getCount(); });
        objectT[sol::meta_function::equal_to] = [](const ObjectT& a, const ObjectT& b) { return a.id() == b.id(); };
        objectT[sol::meta_function::to_string] = &ObjectT::toString;
        objectT["sendEvent"] = [context](const ObjectT& dest, std::string eventName, const sol::object& eventData)
        {
            context.mLocalEventQueue->push_back({dest.id(), std::move(eventName), LuaUtil::serialize(eventData, context.mSerializer)});
        };

        if constexpr (std::is_same_v<ObjectT, GObject>)
        {  // Only for global scripts
            objectT["addScript"] = [luaManager=context.mLuaManager](const GObject& object, const std::string& path)
            {
                luaManager->addLocalScript(object.ptr(), path);
            };

            objectT["teleport"] = [luaManager=context.mLuaManager](const GObject& object, std::string_view cell,
                                                                   const osg::Vec3f& pos, const sol::optional<osg::Vec3f>& rot)
            {
                // TODO
                throw std::logic_error("Not implemented");
            };
        }
    }

    template <class ObjectT>
    static void addDoorBindings(sol::usertype<ObjectT>& objectT, const Context& context)
    {
        auto ptr = [](const ObjectT& o) -> const MWWorld::Ptr& { return requireClass<MWClass::Door>(o.ptr()); };

        objectT["isTeleport"] = sol::readonly_property([ptr](const ObjectT& o)
        {
            return ptr(o).getCellRef().getTeleport();
        });
        objectT["destPosition"] = sol::readonly_property([ptr](const ObjectT& o) -> osg::Vec3f
        {
            return ptr(o).getCellRef().getDoorDest().asVec3();
        });
        objectT["destRotation"] = sol::readonly_property([ptr](const ObjectT& o) -> osg::Vec3f
        {
            return ptr(o).getCellRef().getDoorDest().asRotationVec3();
        });
        objectT["destCell"] = sol::readonly_property([ptr](const ObjectT& o) -> std::string_view
        {
            return ptr(o).getCellRef().getDestCell();
        });
    }

    template <class ObjectT>
    static void addInventoryBindings(sol::usertype<ObjectT>& objectT, const std::string& prefix, const Context& context)
    {
        using InventoryT = Inventory<ObjectT>;
        sol::usertype<InventoryT> inventoryT = context.mLua->sol().new_usertype<InventoryT>(prefix + "Inventory");

        objectT["getEquipment"] = [context](const ObjectT& o)
        {
            const MWWorld::Ptr& ptr = o.ptr();
            MWWorld::InventoryStore& store = ptr.getClass().getInventoryStore(ptr);
            sol::table equipment(context.mLua->sol(), sol::create);
            for (int slot = 0; slot < MWWorld::InventoryStore::Slots; ++slot)
            {
                auto it = store.getSlot(slot);
                if (it == store.end())
                    continue;
                context.mWorldView->getObjectRegistry()->registerPtr(*it);
                equipment[slot] = ObjectT(getId(*it), context.mWorldView->getObjectRegistry());
            }
            return equipment;
        };
        objectT["isEquipped"] = [](const ObjectT& actor, const ObjectT& item)
        {
            const MWWorld::Ptr& ptr = actor.ptr();
            MWWorld::InventoryStore& store = ptr.getClass().getInventoryStore(ptr);
            return store.isEquipped(item.ptr());
        };

        objectT["inventory"] = sol::readonly_property([](const ObjectT& o) { return InventoryT{o}; });
        inventoryT[sol::meta_function::to_string] =
            [](const InventoryT& inv) { return "Inventory[" + inv.mObj.toString() + "]"; };

        auto getWithMask = [context](const InventoryT& inventory, int mask)
        {
            const MWWorld::Ptr& ptr = inventory.mObj.ptr();
            MWWorld::ContainerStore& store = ptr.getClass().getContainerStore(ptr);
            ObjectIdList list = std::make_shared<std::vector<ObjectId>>();
            auto it = store.begin(mask);
            while (it.getType() != -1)
            {
                const MWWorld::Ptr& item = *(it++);
                context.mWorldView->getObjectRegistry()->registerPtr(item);
                list->push_back(getId(item));
            }
            return ObjectList<ObjectT>{list};
        };

        inventoryT["getAll"] =
            [getWithMask](const InventoryT& inventory) { return getWithMask(inventory, MWWorld::ContainerStore::Type_All); };
        inventoryT["getPotions"] =
            [getWithMask](const InventoryT& inventory) { return getWithMask(inventory, MWWorld::ContainerStore::Type_Potion); };
        inventoryT["getApparatuses"] =
            [getWithMask](const InventoryT& inventory) { return getWithMask(inventory, MWWorld::ContainerStore::Type_Apparatus); };
        inventoryT["getArmor"] =
            [getWithMask](const InventoryT& inventory) { return getWithMask(inventory, MWWorld::ContainerStore::Type_Armor); };
        inventoryT["getBooks"] =
            [getWithMask](const InventoryT& inventory) { return getWithMask(inventory, MWWorld::ContainerStore::Type_Book); };
        inventoryT["getClothing"] =
            [getWithMask](const InventoryT& inventory) { return getWithMask(inventory, MWWorld::ContainerStore::Type_Clothing); };
        inventoryT["getIngredients"] =
            [getWithMask](const InventoryT& inventory) { return getWithMask(inventory, MWWorld::ContainerStore::Type_Ingredient); };
        inventoryT["getLights"] =
            [getWithMask](const InventoryT& inventory) { return getWithMask(inventory, MWWorld::ContainerStore::Type_Light); };
        inventoryT["getLockpicks"] =
            [getWithMask](const InventoryT& inventory) { return getWithMask(inventory, MWWorld::ContainerStore::Type_Lockpick); };
        inventoryT["getMiscellaneous"] =
            [getWithMask](const InventoryT& inventory) { return getWithMask(inventory, MWWorld::ContainerStore::Type_Miscellaneous); };
        inventoryT["getProbes"] =
            [getWithMask](const InventoryT& inventory) { return getWithMask(inventory, MWWorld::ContainerStore::Type_Probe); };
        inventoryT["getRepairKits"] =
            [getWithMask](const InventoryT& inventory) { return getWithMask(inventory, MWWorld::ContainerStore::Type_Repair); };
        inventoryT["getWeapons"] =
            [getWithMask](const InventoryT& inventory) { return getWithMask(inventory, MWWorld::ContainerStore::Type_Weapon); };
            
        inventoryT["countOf"] = [](const InventoryT& inventory, const std::string& recordId)
        {
            const MWWorld::Ptr& ptr = inventory.mObj.ptr();
            MWWorld::ContainerStore& store = ptr.getClass().getContainerStore(ptr);
            return store.count(recordId);
        };

        if constexpr (std::is_same_v<ObjectT, GObject>)
        {  // Only for global scripts
            // TODO
            objectT["moveInto"] = [](const GObject& obj, const InventoryT& inventory) {};
            objectT["setEquipment"] = [](const GObject& obj, const sol::table& equipment) {};

            // obj.inventory:drop(obj2, [count])
            // obj.inventory:drop(recordId, [count])
            // obj.inventory:addNew(recordId, [count])
            // obj.inventory:remove(obj/recordId, [count])
            inventoryT["drop"] = [](const InventoryT& inventory) {};
            inventoryT["addNew"] = [](const InventoryT& inventory) {};
            inventoryT["remove"] = [](const InventoryT& inventory) {};
        }
    }

    template <class ObjectT>
    static void initObjectBindings(const std::string& prefix, const Context& context)
    {
        sol::usertype<ObjectT> objectT = context.mLua->sol().new_usertype<ObjectT>(prefix + "Object");
        addBasicBindings<ObjectT>(objectT, context);
        addDoorBindings<ObjectT>(objectT, context);
        addInventoryBindings<ObjectT>(objectT, prefix, context);

        registerObjectList<ObjectT>(prefix, context);
    }

    void initObjectBindingsForLocalScripts(const Context& context)
    {
        initObjectBindings<LObject>("L", context);
    }

    void initObjectBindingsForGlobalScripts(const Context& context)
    {
        initObjectBindings<GObject>("G", context);
    }

}
