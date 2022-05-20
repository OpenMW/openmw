#include "userdataserializer.hpp"

#include <components/lua/serialization.hpp>
#include <components/misc/endianness.hpp>

#include "object.hpp"

namespace MWLua
{

    class Serializer final : public LuaUtil::UserdataSerializer
    {
    public:
        explicit Serializer(bool localSerializer, ObjectRegistry* registry, std::map<int, int>* contentFileMapping)
            : mLocalSerializer(localSerializer), mObjectRegistry(registry), mContentFileMapping(contentFileMapping) {}

    private:
        // Appends serialized sol::userdata to the end of BinaryData.
        // Returns false if this type of userdata is not supported by this serializer.
        bool serialize(LuaUtil::BinaryData& out, const sol::userdata& data) const override
        {
            if (data.is<GObject>() || data.is<LObject>())
            {
                appendRefNum(out, data.as<Object>().id());
                return true;
            }
            return false;
        }

        // Deserializes userdata of type "typeName" from binaryData. Should push the result on stack using sol::stack::push.
        // Returns false if this type is not supported by this serializer.
        bool deserialize(std::string_view typeName, std::string_view binaryData, lua_State* lua) const override
        {
            if (typeName == sRefNumTypeName)
            {
                ObjectId id = loadRefNum(binaryData);
                if (id.hasContentFile() && mContentFileMapping)
                {
                    auto iter = mContentFileMapping->find(id.mContentFile);
                    if (iter != mContentFileMapping->end())
                        id.mContentFile = iter->second;
                }
                if (mLocalSerializer)
                    sol::stack::push<LObject>(lua, LObject(id, mObjectRegistry));
                else
                    sol::stack::push<GObject>(lua, GObject(id, mObjectRegistry));
                return true;
            }
            return false;
        }

        bool mLocalSerializer;
        ObjectRegistry* mObjectRegistry;
        std::map<int, int>* mContentFileMapping;
    };

    std::unique_ptr<LuaUtil::UserdataSerializer> createUserdataSerializer(
        bool local, ObjectRegistry* registry, std::map<int, int>* contentFileMapping)
    {
        return std::make_unique<Serializer>(local, registry, contentFileMapping);
    }

}
