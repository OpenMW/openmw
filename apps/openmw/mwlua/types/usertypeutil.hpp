#ifndef MWLUA_USERTYPEUTIL_H
#define MWLUA_USERTYPEUTIL_H

#include "../contentbindings.hpp"
#include "modelproperty.hpp"

#include "apps/openmw/mwbase/environment.hpp"

#include <components/resource/resourcesystem.hpp>

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

        template <class Data>
        auto operator()(Member<Data, Record> dm, Member<M, Data> member) const
        {
            return [dm, member](const Type& rec) -> const M& {
                const Record& record = asRecord<Record>(rec);
                const Data& data = record.*dm;
                return data.*member;
            };
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

        template <class Data>
        auto operator()(Member<Data, Record> dm, Member<M, Data> member) const
        {
            return [dm, member](MutableRecord<Record>& rec, const M& value) {
                Record& record = rec.find();
                Data& data = record.*dm;
                data.*member = value;
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

        template <class Data>
        auto operator()(Member<Data, Record> dm, Member<ESM::RefId, Data> member) const
        {
            return [dm, member](MutableRecord<Record>& rec, std::string_view value) {
                Record& record = rec.find();
                Data& data = record.*dm;
                data.*member = ESM::RefId::deserializeText(value);
            };
        }
    };

    template <class T>
    constexpr bool isMutable = false;
    template <class T>
    constexpr bool isMutable<MutableRecord<T>> = true;

    template <class Record, class Type, class M>
    void addProperty(sol::usertype<Type>& type, std::string_view key, Member<M, Record> member)
    {
        if constexpr (isMutable<Type>)
            type[key] = sol::property(Getter<Record, Type, M>{}(member), Setter<Record, M>{}(member));
        else
            type[key] = sol::readonly_property(Getter<Record, Type, M>{}(member));
    }

    template <class Record, class Type, class Data, class M>
    void addProperty(sol::usertype<Type>& type, std::string_view key, Member<Data, Record> dm, Member<M, Data> member)
    {
        if constexpr (isMutable<Type>)
            type[key] = sol::property(Getter<Record, Type, M>{}(dm, member), Setter<Record, M>{}(dm, member));
        else
            type[key] = sol::readonly_property(Getter<Record, Type, M>{}(dm, member));
    }

    template <class T>
    void addModelProperty(sol::usertype<T>& type)
    {
        if constexpr (isMutable<T>)
            ::MWLua::addMutableModelProperty(type);
        else
            ::MWLua::addModelProperty(type);
    }

    template <class T>
    void addIconProperty(sol::usertype<T>& type)
    {
        auto vfs = MWBase::Environment::get().getResourceSystem()->getVFS();
        if constexpr (isMutable<T>)
            type["icon"] = sol::property(
                [vfs](const T& mutRec) -> std::string {
                    return Misc::ResourceHelpers::correctIconPath(VFS::Path::toNormalized(mutRec.find().mIcon), *vfs);
                },
                [](T& mutRec, std::string_view path) {
                    auto& recordValue = mutRec.find();
                    recordValue.mIcon = path;
                });
        else
            type["icon"] = sol::readonly_property([vfs](const T& rec) -> std::string {
                return Misc::ResourceHelpers::correctIconPath(VFS::Path::toNormalized(rec.mIcon), *vfs);
            });
    }

    template <class Record>
    Record initFromTemplate(const sol::table& rec)
    {
        if (rec["template"] != sol::nil)
        {
            if (rec["template"].is<MutableRecord<Record>>())
                return rec["template"].get<MutableRecord<Record>>().find();
            else
                return LuaUtil::cast<Record>(rec["template"]);
        }
        Record out;
        out.blank();
        return out;
    }
}

#endif
