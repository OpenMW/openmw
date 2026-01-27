#ifndef MWLUA_CONTENTBINDINGS_H
#define MWLUA_CONTENTBINDINGS_H

#include <sol/forward.hpp>

#include <components/esm/refid.hpp>

#include "../mwworld/store.hpp"

namespace MWLua
{
    struct Context;

    template <class T>
    struct MutableRecord
    {
        MWWorld::Store<T>& mStore;
        ESM::RefId mId;

        const T& find() const { return *mStore.find(mId); }
        T& find() { return *const_cast<T*>(mStore.find(mId)); }
    };

    sol::table initContentPackage(const Context& context);
}

#endif // MWLUA_CONTENTBINDINGS_H
