#ifndef COMPONENTS_LUA_SERIALIZATION_H
#define COMPONENTS_LUA_SERIALIZATION_H

#include <sol/sol.hpp>

#include <components/esm3/cellref.hpp>

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
        virtual bool deserialize(std::string_view typeName, std::string_view binaryData, lua_State*) const = 0;

    protected:
        static void append(BinaryData&, std::string_view typeName, const void* data, size_t dataSize);

        static constexpr std::string_view sRefNumTypeName = "o";
        static void appendRefNum(BinaryData&, ESM::RefNum);
        static ESM::RefNum loadRefNum(std::string_view data);
    };

    // Serializer that can load Lua data from content files and saved games, but doesn't depend on apps/openmw.
    // Instead of LObject/GObject (that are defined in apps/openmw) it loads refnums directly as ESM::RefNum.
    class BasicSerializer final : public UserdataSerializer
    {
    public:
        BasicSerializer() = default;
        explicit BasicSerializer(std::function<int(int)> adjustContentFileIndexFn) :
            mAdjustContentFilesIndexFn(std::move(adjustContentFileIndexFn)) {}

    private:
        bool serialize(LuaUtil::BinaryData& out, const sol::userdata& data) const override;
        bool deserialize(std::string_view typeName, std::string_view binaryData, lua_State* lua) const override;

        std::function<int(int)> mAdjustContentFilesIndexFn;
    };

    BinaryData serialize(const sol::object&, const UserdataSerializer* customSerializer = nullptr);
    sol::object deserialize(lua_State* lua, std::string_view binaryData,
                            const UserdataSerializer* customSerializer = nullptr, bool readOnly = false);

}

#endif // COMPONENTS_LUA_SERIALIZATION_H
