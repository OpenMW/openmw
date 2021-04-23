#include "serialization.hpp"

#include <osg/Vec2f>
#include <osg/Vec3f>

#include <components/misc/endianness.hpp>

namespace LuaUtil
{

    constexpr unsigned char FORMAT_VERSION = 0;

    enum class SerializedType : char
    {
        NUMBER =       0x0,
        LONG_STRING =  0x1,
        BOOLEAN =      0x2,
        TABLE_START =  0x3,
        TABLE_END =    0x4,

        VEC2 =         0x10,
        VEC3 =         0x11,

        // All values should be lesser than 0x20 (SHORT_STRING_FLAG).
    };
    constexpr unsigned char SHORT_STRING_FLAG = 0x20;    // 0b001SSSSS. SSSSS = string length
    constexpr unsigned char CUSTOM_FULL_FLAG = 0x40;     // 0b01TTTTTT + 32bit dataSize
    constexpr unsigned char CUSTOM_COMPACT_FLAG = 0x80;  // 0b1SSSSTTT. SSSS = dataSize, TTT = (typeName size - 1)

    static void appendType(BinaryData& out, SerializedType type)
    {
        out.push_back(static_cast<char>(type));
    }

    template <typename T>
    static void appendValue(BinaryData& out, T v)
    {
        v = Misc::toLittleEndian(v);
        out.append(reinterpret_cast<const char*>(&v), sizeof(v));
    }

    template <typename T>
    static T getValue(std::string_view& binaryData)
    {
        if (binaryData.size() < sizeof(T))
            throw std::runtime_error("Unexpected end");
        T v;
        std::memcpy(&v, binaryData.data(), sizeof(T));
        binaryData = binaryData.substr(sizeof(T));
        return Misc::fromLittleEndian(v);
    }

    static void appendString(BinaryData& out, std::string_view str)
    {
        if (str.size() < 32)
            out.push_back(SHORT_STRING_FLAG | char(str.size()));
        else
        {
            appendType(out, SerializedType::LONG_STRING);
            appendValue<uint32_t>(out, str.size());
        }
        out.append(str.data(), str.size());
    }

    static void appendData(BinaryData& out, const void* data, size_t dataSize)
    {
        out.append(reinterpret_cast<const char*>(data), dataSize);
    }

    void UserdataSerializer::append(BinaryData& out, std::string_view typeName, const void* data, size_t dataSize)
    {
        assert(!typeName.empty() && typeName.size() <= 64);
        if (typeName.size() <= 8 && dataSize < 16)
        {  // Compact form: 0b1SSSSTTT. SSSS = dataSize, TTT = (typeName size - 1).
            unsigned char t = CUSTOM_COMPACT_FLAG | (dataSize << 3) | (typeName.size() - 1);
            out.push_back(t);
        }
        else
        {  // Full form: 0b01TTTTTT + 32bit dataSize.
            unsigned char t = CUSTOM_FULL_FLAG | (typeName.size() - 1);
            out.push_back(t);
            appendValue<uint32_t>(out, dataSize);
        }
        out.append(typeName.data(), typeName.size());
        appendData(out, data, dataSize);
    }

    static void serializeUserdata(BinaryData& out, const sol::userdata& data, const UserdataSerializer* customSerializer)
    {
        if (data.is<osg::Vec2f>())
        {
            appendType(out, SerializedType::VEC2);
            osg::Vec2f v = data.as<osg::Vec2f>();
            appendValue<float>(out, v.x());
            appendValue<float>(out, v.y());
            return;
        }
        if (data.is<osg::Vec3f>())
        {
            appendType(out, SerializedType::VEC3);
            osg::Vec3f v = data.as<osg::Vec3f>();
            appendValue<float>(out, v.x());
            appendValue<float>(out, v.y());
            appendValue<float>(out, v.z());
            return;
        }
        if (customSerializer && customSerializer->serialize(out, data))
            return;
        else
            throw std::runtime_error("Unknown userdata");
    }

    static void serialize(BinaryData& out, const sol::object& obj, const UserdataSerializer* customSerializer, int recursionCounter)
    {
        if (obj.get_type() == sol::type::lightuserdata)
            throw std::runtime_error("light userdata is not allowed to be serialized");
        if (obj.is<sol::function>())
            throw std::runtime_error("functions are not allowed to be serialized");
        else if (obj.is<sol::userdata>())
            serializeUserdata(out, obj, customSerializer);
        else if (obj.is<sol::lua_table>())
        {
            if (recursionCounter >= 32)
                throw std::runtime_error("Can not serialize more than 32 nested tables. Likely the table contains itself.");
            sol::table table = obj;
            appendType(out, SerializedType::TABLE_START);
            for (auto& [key, value] : table)
            {
                serialize(out, key, customSerializer, recursionCounter + 1);
                serialize(out, value, customSerializer, recursionCounter + 1);
            }
            appendType(out, SerializedType::TABLE_END);
        }
        else if (obj.is<double>())
        {
            appendType(out, SerializedType::NUMBER);
            appendValue<double>(out, obj.as<double>());
        }
        else if (obj.is<std::string_view>())
            appendString(out, obj.as<std::string_view>());
        else if (obj.is<bool>())
        {
            char v = obj.as<bool>() ? 1 : 0;
            appendType(out, SerializedType::BOOLEAN);
            out.push_back(v);
        } else
            throw std::runtime_error("Unknown lua type");
    }

    static void deserializeImpl(sol::state& lua, std::string_view& binaryData, const UserdataSerializer* customSerializer)
    {
        if (binaryData.empty())
            throw std::runtime_error("Unexpected end");
        unsigned char type = binaryData[0];
        binaryData = binaryData.substr(1);
        if (type & (CUSTOM_COMPACT_FLAG | CUSTOM_FULL_FLAG))
        {
            size_t typeNameSize, dataSize;
            if (type & CUSTOM_COMPACT_FLAG)
            {  // Compact form: 0b1SSSSTTT. SSSS = dataSize, TTT = (typeName size - 1).
                typeNameSize = (type & 7) + 1;
                dataSize = (type >> 3) & 15;
            }
            else
            {  // Full form: 0b01TTTTTT + 32bit dataSize.
                typeNameSize = (type & 63) + 1;
                dataSize = getValue<uint32_t>(binaryData);
            }
            std::string_view typeName = binaryData.substr(0, typeNameSize);
            std::string_view data = binaryData.substr(typeNameSize, dataSize);
            binaryData = binaryData.substr(typeNameSize + dataSize);
            if (!customSerializer || !customSerializer->deserialize(typeName, data, lua))
                throw std::runtime_error("Unknown type: " + std::string(typeName));
            return;
        }
        if (type & SHORT_STRING_FLAG)
        {
            size_t size = type & 0x1f;
            sol::stack::push<std::string_view>(lua.lua_state(), binaryData.substr(0, size));
            binaryData = binaryData.substr(size);
            return;
        }
        switch (static_cast<SerializedType>(type))
        {
            case SerializedType::NUMBER:
                sol::stack::push<double>(lua.lua_state(), getValue<double>(binaryData));
                return;
            case SerializedType::BOOLEAN:
                sol::stack::push<bool>(lua.lua_state(), getValue<char>(binaryData) != 0);
                return;
            case SerializedType::LONG_STRING:
            {
                uint32_t size = getValue<uint32_t>(binaryData);
                sol::stack::push<std::string_view>(lua.lua_state(), binaryData.substr(0, size));
                binaryData = binaryData.substr(size);
                return;
            }
            case SerializedType::TABLE_START:
            {
                lua_createtable(lua, 0, 0);
                while (!binaryData.empty() && binaryData[0] != char(SerializedType::TABLE_END))
                {
                    deserializeImpl(lua, binaryData, customSerializer);
                    deserializeImpl(lua, binaryData, customSerializer);
                    lua_settable(lua, -3);
                }
                if (binaryData.empty())
                    throw std::runtime_error("Unexpected end");
                binaryData = binaryData.substr(1);
                return;
            }
            case SerializedType::TABLE_END:
                throw std::runtime_error("Unexpected table end");
            case SerializedType::VEC2:
            {
                float x = getValue<float>(binaryData);
                float y = getValue<float>(binaryData);
                sol::stack::push<osg::Vec2f>(lua.lua_state(), osg::Vec2f(x, y));
                return;
            }
            case SerializedType::VEC3:
            {
                float x = getValue<float>(binaryData);
                float y = getValue<float>(binaryData);
                float z = getValue<float>(binaryData);
                sol::stack::push<osg::Vec3f>(lua.lua_state(), osg::Vec3f(x, y, z));
                return;
            }
        }
        throw std::runtime_error("Unknown type: " + std::to_string(type));
    }

    BinaryData serialize(const sol::object& obj, const UserdataSerializer* customSerializer)
    {
        if (obj == sol::nil)
            return "";
        BinaryData res;
        res.push_back(FORMAT_VERSION);
        serialize(res, obj, customSerializer, 0);
        return res;
    }

    sol::object deserialize(sol::state& lua, std::string_view binaryData, const UserdataSerializer* customSerializer)
    {
        if (binaryData.empty())
            return sol::nil;
        if (binaryData[0] != FORMAT_VERSION)
            throw std::runtime_error("Incorrect version of Lua serialization format: " +
                                     std::to_string(static_cast<unsigned>(binaryData[0])));
        binaryData = binaryData.substr(1);
        deserializeImpl(lua, binaryData, customSerializer);
        if (!binaryData.empty())
            throw std::runtime_error("Unexpected data after serialized object");
        return sol::stack::pop<sol::object>(lua.lua_state());
    }

}
