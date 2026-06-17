#include "serialization.hpp"

#include <osg/Matrixf>
#include <osg/Quat>
#include <osg/Vec2f>
#include <osg/Vec3f>
#include <osg/Vec4f>

#include <components/misc/color.hpp>
#include <components/misc/endianness.hpp>

#include "luastate.hpp"
#include "utilpackage.hpp"

namespace LuaUtil
{

    constexpr unsigned char FORMAT_VERSION = 0;

    enum class SerializedType : char
    {
        NUMBER = 0x0,
        LONG_STRING = 0x1,
        BOOLEAN = 0x2,
        TABLE_START = 0x3,
        TABLE_END = 0x4,

        VEC2 = 0x10,
        VEC3 = 0x11,
        TRANSFORM_M = 0x12,
        TRANSFORM_Q = 0x13,
        VEC4 = 0x14,
        COLOR = 0x15,

        // All values should be lesser than 0x20 (SHORT_STRING_FLAG).
    };
    constexpr unsigned char SHORT_STRING_FLAG = 0x20; // 0b001SSSSS. SSSSS = string length
    constexpr unsigned char CUSTOM_FULL_FLAG = 0x40; // 0b01TTTTTT + 32bit dataSize
    constexpr unsigned char CUSTOM_COMPACT_FLAG = 0x80; // 0b1SSSSTTT. SSSS = dataSize, TTT = (typeName size - 1)

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
            throw std::runtime_error("Unexpected end of serialized data.");
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
            appendValue<uint32_t>(out, static_cast<uint32_t>(str.size()));
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
        { // Compact form: 0b1SSSSTTT. SSSS = dataSize, TTT = (typeName size - 1).
            auto t = static_cast<unsigned char>(CUSTOM_COMPACT_FLAG | (dataSize << 3) | (typeName.size() - 1));
            out.push_back(t);
        }
        else
        { // Full form: 0b01TTTTTT + 32bit dataSize.
            auto t = static_cast<unsigned char>(CUSTOM_FULL_FLAG | (typeName.size() - 1));
            out.push_back(t);
            appendValue<uint32_t>(out, static_cast<uint32_t>(dataSize));
        }
        out.append(typeName.data(), typeName.size());
        appendData(out, data, dataSize);
    }

    void UserdataSerializer::appendRefNum(BinaryData& out, ESM::RefNum refnum)
    {
        static_assert(sizeof(ESM::RefNum) == 8);
        refnum.mIndex = Misc::toLittleEndian(refnum.mIndex);
        refnum.mContentFile = Misc::toLittleEndian(refnum.mContentFile);
        append(out, sRefNumTypeName, &refnum, sizeof(ESM::RefNum));
    }

    bool BasicSerializer::serialize(BinaryData& out, const sol::userdata& data) const
    {
        appendRefNum(out, cast<ESM::RefNum>(data));
        return true;
    }

    bool BasicSerializer::deserialize(std::string_view typeName, std::string_view binaryData, lua_State* lua) const
    {
        if (typeName != sRefNumTypeName)
            return false;
        ESM::RefNum refnum = loadRefNum(binaryData);
        if (mAdjustContentFilesIndexFn)
            refnum.mContentFile = mAdjustContentFilesIndexFn(refnum.mContentFile);
        sol::stack::push<ESM::RefNum>(lua, refnum);
        return true;
    }

    ESM::RefNum UserdataSerializer::loadRefNum(std::string_view data)
    {
        if (data.size() != sizeof(ESM::RefNum))
            throw std::runtime_error("Incorrect serialization format. Size of RefNum doesn't match.");
        ESM::RefNum refnum;
        std::memcpy(&refnum, data.data(), sizeof(ESM::RefNum));
        refnum.mIndex = Misc::fromLittleEndian(refnum.mIndex);
        refnum.mContentFile = Misc::fromLittleEndian(refnum.mContentFile);
        return refnum;
    }

    static void serializeUserdata(
        BinaryData& out, const sol::userdata& data, const UserdataSerializer* customSerializer)
    {
        if (data.is<osg::Vec2f>())
        {
            appendType(out, SerializedType::VEC2);
            osg::Vec2f v = data.as<osg::Vec2f>();
            appendValue<double>(out, v.x());
            appendValue<double>(out, v.y());
            return;
        }
        if (data.is<osg::Vec3f>())
        {
            appendType(out, SerializedType::VEC3);
            osg::Vec3f v = data.as<osg::Vec3f>();
            appendValue<double>(out, v.x());
            appendValue<double>(out, v.y());
            appendValue<double>(out, v.z());
            return;
        }
        if (data.is<TransformM>())
        {
            appendType(out, SerializedType::TRANSFORM_M);
            osg::Matrixf matrix = data.as<TransformM>().mM;
            for (int i = 0; i < 4; i++)
                for (int j = 0; j < 4; j++)
                    appendValue<double>(out, matrix(i, j));
            return;
        }
        if (data.is<TransformQ>())
        {
            appendType(out, SerializedType::TRANSFORM_Q);
            osg::Quat quat = data.as<TransformQ>().mQ;
            for (int i = 0; i < 4; i++)
                appendValue<double>(out, quat[i]);
            return;
        }
        if (data.is<osg::Vec4f>())
        {
            appendType(out, SerializedType::VEC4);
            osg::Vec4f v = data.as<osg::Vec4f>();
            appendValue<double>(out, v.x());
            appendValue<double>(out, v.y());
            appendValue<double>(out, v.z());
            appendValue<double>(out, v.w());
            return;
        }
        if (data.is<Misc::Color>())
        {
            appendType(out, SerializedType::COLOR);
            Misc::Color v = data.as<Misc::Color>();
            appendValue<float>(out, v.r());
            appendValue<float>(out, v.g());
            appendValue<float>(out, v.b());
            appendValue<float>(out, v.a());
            return;
        }
        if (customSerializer && customSerializer->serialize(out, data))
            return;
        else
            throw std::runtime_error("Value is not serializable.");
    }

    static void serialize(
        BinaryData& out, const sol::object& obj, const UserdataSerializer* customSerializer, int recursionCounter)
    {
        if (obj.get_type() == sol::type::lightuserdata)
            throw std::runtime_error("Light userdata is not allowed to be serialized.");
        if (obj.is<sol::function>())
            throw std::runtime_error("Functions are not allowed to be serialized.");
        else if (obj.is<sol::userdata>())
            serializeUserdata(out, obj, customSerializer);
        else if (obj.is<sol::lua_table>())
        {
            if (recursionCounter >= 32)
                throw std::runtime_error(
                    "Can not serialize more than 32 nested tables. Likely the table contains itself.");
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
        }
        else
            throw std::runtime_error("Unknown Lua type.");
    }

    static void deserializeImpl(
        lua_State* lua, std::string_view& binaryData, const UserdataSerializer* customSerializer, bool readOnly)
    {
        if (binaryData.empty())
            throw std::runtime_error("Unexpected end of serialized data.");
        unsigned char type = binaryData[0];
        binaryData = binaryData.substr(1);
        if (type & (CUSTOM_COMPACT_FLAG | CUSTOM_FULL_FLAG))
        {
            size_t typeNameSize, dataSize;
            if (type & CUSTOM_COMPACT_FLAG)
            { // Compact form: 0b1SSSSTTT. SSSS = dataSize, TTT = (typeName size - 1).
                typeNameSize = (type & 7) + 1;
                dataSize = (type >> 3) & 15;
            }
            else
            { // Full form: 0b01TTTTTT + 32bit dataSize.
                typeNameSize = (type & 63) + 1;
                dataSize = getValue<uint32_t>(binaryData);
            }
            std::string_view typeName = binaryData.substr(0, typeNameSize);
            std::string_view data = binaryData.substr(typeNameSize, dataSize);
            binaryData = binaryData.substr(typeNameSize + dataSize);
            if (!customSerializer || !customSerializer->deserialize(typeName, data, lua))
                throw std::runtime_error("Unknown type in serialized data: " + std::string(typeName));
            return;
        }
        if (type & SHORT_STRING_FLAG)
        {
            size_t size = type & 0x1f;
            sol::stack::push<std::string_view>(lua, binaryData.substr(0, size));
            binaryData = binaryData.substr(size);
            return;
        }
        switch (static_cast<SerializedType>(type))
        {
            case SerializedType::NUMBER:
                sol::stack::push<double>(lua, getValue<double>(binaryData));
                return;
            case SerializedType::BOOLEAN:
                sol::stack::push<bool>(lua, getValue<char>(binaryData) != 0);
                return;
            case SerializedType::LONG_STRING:
            {
                uint32_t size = getValue<uint32_t>(binaryData);
                sol::stack::push<std::string_view>(lua, binaryData.substr(0, size));
                binaryData = binaryData.substr(size);
                return;
            }
            case SerializedType::TABLE_START:
            {
                lua_createtable(lua, 0, 0);
                while (!binaryData.empty() && binaryData[0] != char(SerializedType::TABLE_END))
                {
                    deserializeImpl(lua, binaryData, customSerializer, readOnly);
                    deserializeImpl(lua, binaryData, customSerializer, readOnly);
                    lua_settable(lua, -3);
                }
                if (binaryData.empty())
                    throw std::runtime_error("Unexpected end of serialized data.");
                binaryData = binaryData.substr(1);
                if (readOnly)
                    sol::stack::push(lua, makeReadOnly(sol::stack::pop<sol::table>(lua)));
                return;
            }
            case SerializedType::TABLE_END:
                throw std::runtime_error("Unexpected end of table during deserialization.");
            case SerializedType::VEC2:
            {
                float x = static_cast<float>(getValue<double>(binaryData));
                float y = static_cast<float>(getValue<double>(binaryData));
                sol::stack::push<osg::Vec2f>(lua, osg::Vec2f(x, y));
                return;
            }
            case SerializedType::VEC3:
            {
                float x = static_cast<float>(getValue<double>(binaryData));
                float y = static_cast<float>(getValue<double>(binaryData));
                float z = static_cast<float>(getValue<double>(binaryData));
                sol::stack::push<osg::Vec3f>(lua, osg::Vec3f(x, y, z));
                return;
            }
            case SerializedType::TRANSFORM_M:
            {
                osg::Matrixf mat;
                for (int i = 0; i < 4; i++)
                    for (int j = 0; j < 4; j++)
                        mat(i, j) = static_cast<float>(getValue<double>(binaryData));
                sol::stack::push<TransformM>(lua, asTransform(mat));
                return;
            }
            case SerializedType::TRANSFORM_Q:
            {
                osg::Quat q;
                for (int i = 0; i < 4; i++)
                    q[i] = getValue<double>(binaryData);
                sol::stack::push<TransformQ>(lua, asTransform(q));
                return;
            }
            case SerializedType::VEC4:
            {
                float x = static_cast<float>(getValue<double>(binaryData));
                float y = static_cast<float>(getValue<double>(binaryData));
                float z = static_cast<float>(getValue<double>(binaryData));
                float w = static_cast<float>(getValue<double>(binaryData));
                sol::stack::push<osg::Vec4f>(lua, osg::Vec4f(x, y, z, w));
                return;
            }
            case SerializedType::COLOR:
            {
                float r = getValue<float>(binaryData);
                float g = getValue<float>(binaryData);
                float b = getValue<float>(binaryData);
                float a = getValue<float>(binaryData);
                sol::stack::push<Misc::Color>(lua, Misc::Color(r, g, b, a));
                return;
            }
        }
        throw std::runtime_error("Unknown type in serialized data: " + std::to_string(type));
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

    sol::object deserialize(
        lua_State* lua, std::string_view binaryData, const UserdataSerializer* customSerializer, bool readOnly)
    {
        if (binaryData.empty())
            return sol::nil;
        if (binaryData[0] != FORMAT_VERSION)
            throw std::runtime_error("Incorrect version of Lua serialization format: "
                + std::to_string(static_cast<unsigned>(binaryData[0])));
        binaryData = binaryData.substr(1);
        deserializeImpl(lua, binaryData, customSerializer, readOnly);
        if (!binaryData.empty())
            throw std::runtime_error("Unexpected data after serialized object");
        return sol::stack::pop<sol::object>(lua);
    }

}
