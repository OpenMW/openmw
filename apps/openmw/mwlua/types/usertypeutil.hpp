#ifndef MWLUA_USERTYPEUTIL_H
#define MWLUA_USERTYPEUTIL_H

#include "../contentbindings.hpp"

namespace MWLua::Types
{
    template <class Record, class Type>
    const Record& asRecord(const Type& rec)
    {
        return rec;
    }

    template <class Record>
    const Record& asRecord(const MutableRecord<Record>& rec)
    {
        return rec.find();
    }

    template <class R, class T>
    using Member = R T::*;

    template <class Record, class Type, class M>
    struct Getter
    {
        auto operator()(Member<M, Record> member) const
        {
            return [member](const Type& rec) -> const M& { return asRecord<Record>(rec).*member; };
        }
    };

    template <class Record, class M>
    struct Setter
    {
        auto operator()(Member<M, Record> member) const
        {
            return [member](MutableRecord<Record>& rec, const M& value) {
                Record& record = rec.find();
                record.*member = value;
            };
        }
    };

    template <class Record>
    struct Setter<Record, ESM::RefId>
    {
        auto operator()(Member<ESM::RefId, Record> member) const
        {
            return [member](MutableRecord<Record>& rec, std::string_view value) {
                Record& record = rec.find();
                record.*member = ESM::RefId::deserializeText(value);
            };
        }
    };

    template <class Record, class Type, class M>
    void addProperty(sol::usertype<Type>& type, std::string_view key, Member<M, Record> member)
    {
        constexpr bool isMutable = std::is_same_v<Type, MutableRecord<Record>>;
        if constexpr (isMutable)
            type[key] = sol::property(Getter<Record, Type, M>{}(member), Setter<Record, M>{}(member));
        else
            type[key] = sol::readonly_property(Getter<Record, Type, M>{}(member));
    }
}

#endif
