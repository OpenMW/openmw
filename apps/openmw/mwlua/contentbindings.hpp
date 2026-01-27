#ifndef MWLUA_CONTENTBINDINGS_H
#define MWLUA_CONTENTBINDINGS_H

#include <sol/forward.hpp>

#include <components/esm/refid.hpp>

namespace MWWorld
{
    template <class T>
    class Store;
}

namespace MWLua
{
    struct Context;

    template <class T>
    struct MutableRecord
    {
        MWWorld::Store<T>& mStore;
        ESM::RefId mId;

        const T& find() const;
        T& find();
    };

    sol::table initContentPackage(const Context& context);
}

namespace sol
{
    template <class T>
    struct is_automagical<MWLua::MutableRecord<T>> : std::false_type
    {
    };
}

#endif // MWLUA_CONTENTBINDINGS_H
