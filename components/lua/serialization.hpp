#ifndef COMPONENTS_LUA_SERIALIZATION_H
#define COMPONENTS_LUA_SERIALIZATION_H

#include <limits> // missing from sol/sol.hpp
#include <sol/sol.hpp>

namespace LuaUtil
{

    // Note: it can contain \0
    using BinaryData = std::string;

    class UserdataSerializer
    {
    public:
        virtual ~UserdataSerializer() {}

        // Appends serialized sol::userdata to the end of BinaryData.
        // Returns false if this type of userdata is not supported by this serializer.
        virtual bool serialize(BinaryData&, const sol::userdata&) const = 0;

        // Deserializes userdata of type "typeName" from binaryData. Should push the result on stack using sol::stack::push.
        // Returns false if this type is not supported by this serializer.
        virtual bool deserialize(std::string_view typeName, std::string_view binaryData, sol::state&) const = 0;

    protected:
        static void append(BinaryData&, std::string_view typeName, const void* data, size_t dataSize);
    };

    BinaryData serialize(const sol::object&, const UserdataSerializer* customSerializer = nullptr);
    sol::object deserialize(sol::state& lua, std::string_view binaryData, const UserdataSerializer* customSerializer = nullptr);

}

#endif // COMPONENTS_LUA_SERIALIZATION_H
