#ifndef MWLUA_USERTYPEUTIL_H
#define MWLUA_USERTYPEUTIL_H

#include "../contentbindings.hpp"
#include "modelproperty.hpp"

#include "apps/openmw/mwbase/environment.hpp"

#include <components/misc/finitevalues.hpp>
#include <components/resource/resourcesystem.hpp>

#include <concepts>

namespace MWLua::Types
{
    template <class Type, class M>
    struct Setter
    {
        template <class Accessor>
        auto operator()(Accessor&& accessor) const
        {
            return [=](Type& rec, const M& value) { accessor(rec) = value; };
        }
    };

    template <class Type>
    struct Setter<Type, ESM::RefId>
    {
        template <class Accessor>
        auto operator()(Accessor&& accessor) const
        {
            return [=](Type& rec, std::optional<std::string_view> value) {
                accessor(rec) = ESM::RefId::deserializeText(value.value_or(std::string_view()));
            };
        }
    };

    template <class Type>
    struct Setter<Type, std::string>
    {
        template <class Accessor>
        auto operator()(Accessor&& accessor) const
        {
            return [=](Type& rec, std::string_view value) { accessor(rec) = value; };
        }
    };

    template <class Type, std::floating_point Float>
    struct Setter<Type, Float>
    {
        template <class Accessor>
        auto operator()(Accessor&& accessor) const
        {
            return [=](Type& rec, Misc::FiniteValue<Float> value) { accessor(rec) = value; };
        }
    };

    template <class T>
    struct RecordType
    {
        using Record = T;
        constexpr static bool isMutable = false;

        static const Record& asRecord(const T& rec) { return rec; }
    };

    template <class T>
    struct RecordType<MutableRecord<T>>
    {
        using Record = T;
        constexpr static bool isMutable = true;

        static const Record& asRecord(const MutableRecord<T>& rec) { return rec.find(); }
    };

    template <class Type, class... Member>
    void addProperty(sol::usertype<Type>& type, std::string_view key, Member... members)
    {
        using Record = RecordType<Type>::Record;
        const auto getter = [=](const Type& rec) -> const auto&
        {
            const Record& record = RecordType<Type>::asRecord(rec);
            return (record.*....*members);
        };
        using MemberType = std::remove_cvref_t<std::invoke_result_t<decltype(getter), const Type&>>;
        if constexpr (RecordType<Type>::isMutable)
            type[key] = sol::property(std::move(getter), Setter<Type, MemberType>{}([=](Type& rec) -> MemberType& {
                Record& record = rec.find();
                return (record.*....*members);
            }));
        else
            type[key] = sol::readonly_property(std::move(getter));
    }

    template <class Type, class Flag, class... Member>
    void addFlagProperty(sol::usertype<Type>& type, std::string_view key, Flag flag, Member... members)
    {
        using Record = RecordType<Type>::Record;
        const auto getter = [=](const Type& rec) -> bool {
            const Record& record = RecordType<Type>::asRecord(rec);
            return (record.*....*members) & flag;
        };
        if constexpr (RecordType<Type>::isMutable)
            type[key] = sol::property(std::move(getter), [=](Type& rec, bool value) {
                Record& record = rec.find();
                auto& data = (record.*....*members);
                if (value)
                    data |= flag;
                else
                    data &= ~flag;
            });
        else
            type[key] = sol::readonly_property(std::move(getter));
    }

    template <class Type, class Flag, class... Member>
    void addReverseFlagProperty(sol::usertype<Type>& type, std::string_view key, Flag flag, Member... members)
    {
        using Record = RecordType<Type>::Record;
        const auto getter = [=](const Type& rec) -> bool {
            const Record& record = RecordType<Type>::asRecord(rec);
            return !((record.*....*members) & flag);
        };
        if constexpr (RecordType<Type>::isMutable)
            type[key] = sol::property(std::move(getter), [=](Type& rec, bool value) {
                Record& record = rec.find();
                auto& data = (record.*....*members);
                if (value)
                    data &= ~flag;
                else
                    data |= flag;
            });
        else
            type[key] = sol::readonly_property(std::move(getter));
    }

    template <class T>
    void addModelProperty(sol::usertype<T>& type)
    {
        if constexpr (RecordType<T>::isMutable)
            ::MWLua::addMutableModelProperty(type);
        else
            ::MWLua::addModelProperty(type);
    }

    template <class T>
    void addIconProperty(sol::usertype<T>& type)
    {
        auto vfs = MWBase::Environment::get().getResourceSystem()->getVFS();
        if constexpr (RecordType<T>::isMutable)
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
