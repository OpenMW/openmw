#ifndef MWLUA_OBJECTVARIANT_H
#define MWLUA_OBJECTVARIANT_H

#include <variant>

#include "localscripts.hpp"
#include "object.hpp"

namespace MWLua
{

    class ObjectVariant
    {
    public:
        explicit ObjectVariant(const sol::object& obj)
        {
            if (obj.is<SelfObject>())
                mVariant.emplace<SelfObject*>(obj.as<SelfObject*>());
            else if (obj.is<LObject>())
                mVariant.emplace<LObject>(obj.as<LObject>());
            else
                mVariant.emplace<GObject>(obj.as<GObject>());
        }

        bool isSelfObject() const { return std::holds_alternative<SelfObject*>(mVariant); }
        bool isLObject() const { return std::holds_alternative<LObject>(mVariant); }
        bool isGObject() const { return std::holds_alternative<GObject>(mVariant); }

        SelfObject* asSelfObject() const
        {
            if (!isSelfObject())
                throw std::runtime_error("Allowed only in local scripts for 'openmw.self'.");
            return std::get<SelfObject*>(mVariant);
        }

        const MWWorld::Ptr& ptr() const
        {
            return std::visit(
                [](auto&& variant) -> const MWWorld::Ptr& {
                    using T = std::decay_t<decltype(variant)>;
                    if constexpr (std::is_same_v<T, SelfObject*>)
                        return variant->ptr();
                    else
                        return variant.ptr();
                },
                mVariant);
        }

    private:
        std::variant<SelfObject*, LObject, GObject> mVariant;
    };

} // namespace MWLua

#endif // MWLUA_OBJECTVARIANT_H
