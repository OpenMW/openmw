#include "luabindings.hpp"

#include <components/lua/luastate.hpp>

#include "eventqueue.hpp"
#include "luamanagerimp.hpp"

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
}

namespace MWLua
{

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
        }
    }

    template <class ObjectT>
    static void initObjectBindings(const std::string& prefix, const Context& context)
    {
        sol::usertype<ObjectT> objectT = context.mLua->sol().new_usertype<ObjectT>(prefix + "Object");
        addBasicBindings<ObjectT>(objectT, context);

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

