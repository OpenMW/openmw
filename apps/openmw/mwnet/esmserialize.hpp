#ifndef ESMSERIALIZE_H_
#define ESMSERIALIZE_H_

#include <cstdint>
#include <limits>
#include <map>

#include <osg/Vec3f>
#include <serialize.h>

#include <components/esm/position.hpp>
#include <components/esm/refid.hpp>
#include <components/esm3/cellref.hpp>
#include <components/lua/serialization.hpp>

#include "../mwmechanics/stat.hpp"

constexpr std::size_t MAX_STRING_LENGTH = 256 + sizeof(int) + 1;
constexpr std::size_t MAX_LUADATA_SIZE = 1024 * 512;

namespace serialize
{
    // OSG::Vec3f
    template <typename Stream>
    bool serialize_vec3f_internal(Stream& stream, osg::Vec3f& vec3)
    {
        serialize_float(stream, vec3.x());
        serialize_float(stream, vec3.y());
        serialize_float(stream, vec3.z());
        return true;
    }

    /**
        DREAMWEAVE SERIALIZATION:
        Serialize an osg::Vec3f to the stream (read/write/measure).
        This is a helper macro to make writing unified serialize functions easier.
        Serialize macros returns false on error so we don't need to use exceptions for error handling on read. This is
       an important safety measure because packet data comes from the network and may be malicious. IMPORTANT: This
       macro must be called inside a templated serialize function with template \<typename Stream\>. The serialize
       method must have a bool return value.
        @param stream The stream object. May be a read, write or measure stream.
        @param vec3 An instance of an osg::Vec3f to read/write.
     */

#define serialize_vec3f(stream, vec3f)                                                                                 \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!serialize::serialize_vec3f_internal(stream, vec3f))                                                       \
        {                                                                                                              \
            return false;                                                                                              \
        }                                                                                                              \
    } while (0)

    // ESM::Position
    template <typename Stream>
    bool serialize_esm_position_internal(Stream& stream, ESM::Position& position)
    {
        serialize_float(stream, position.pos[0]);
        serialize_float(stream, position.pos[1]);
        serialize_float(stream, position.pos[2]);
        serialize_float(stream, position.rot[0]);
        serialize_float(stream, position.rot[1]);
        serialize_float(stream, position.rot[2]);
        return true;
    }

    /**
        DREAMWEAVE SERIALIZATION:
        Serialize an ESM::Position to the stream (read/write/measure).
        This is a helper macro to make writing unified serialize functions easier.
        Serialize macros returns false on error so we don't need to use exceptions for error handling on read. This is
       an important safety measure because packet data comes from the network and may be malicious. IMPORTANT: This
       macro must be called inside a templated serialize function with template \<typename Stream\>. The serialize
       method must have a bool return value.
        @param stream The stream object. May be a read, write or measure stream.
        @param position The object position. Note that ESM::Position is composed of position and rotation.
     */

#define serialize_esm_position(stream, position)                                                                       \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!serialize::serialize_esm_position_internal(stream, position))                                             \
        {                                                                                                              \
            return false;                                                                                              \
        }                                                                                                              \
    } while (0)

    // ESM::RefId
    template <typename Stream>
    bool serialize_ref_id_internal(Stream& stream, ESM::RefId& refId)
    {
        if (Stream::IsWriting)
        {
            std::string refIdString = refId.getRefIdString();
            serialize_std_string_internal(stream, refIdString, MAX_STRING_LENGTH);
        }

        if (Stream::IsReading)
        {
            std::string string;
            serialize_std_string_internal(stream, string, MAX_STRING_LENGTH);
            refId = ESM::RefId::stringRefId(string);
        }

        return true;
    }

    /**
        DREAMWEAVE SERIALIZATION:
        Serialize an ESM::RefId to the stream (read/write/measure).
        This is a helper macro to make writing unified serialize functions easier.
        Serialize macros returns false on error so we don't need to use exceptions for error handling on read. This is
       an important safety measure because packet data comes from the network and may be malicious. IMPORTANT: This
       macro must be called inside a templated serialize function with template \<typename Stream\>. The serialize
       method must have a bool return value.
        @param stream The stream object. May be a read, write or measure stream.
        @param refid The object's id. Internally this serializer uses the toString method of ESM::RefId.
     */

#define serialize_ref_id(stream, refId)                                                                                \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!serialize::serialize_ref_id_internal(stream, refId))                                                      \
        {                                                                                                              \
            return false;                                                                                              \
        }                                                                                                              \
    } while (0)

    // ESM::RefNum
    template <typename Stream>
    bool serialize_ref_num_internal(Stream& stream, ESM::RefNum& refNum)
    {
        serialize_int(stream, refNum.mContentFile, 0, std::numeric_limits<int32_t>::max());
        serialize_int(stream, refNum.mIndex, 0, std::numeric_limits<uint32_t>::max());
        return true;
    }

    /**
        DREAMWEAVE SERIALIZATION:
        Serialize an ESM::RefNum to the stream (read/write/measure).
        This is a helper macro to make writing unified serialize functions easier.
        Serialize macros returns false on error so we don't need to use exceptions for error handling on read. This is
       an important safety measure because packet data comes from the network and may be malicious. IMPORTANT: This
       macro must be called inside a templated serialize function with template \<typename Stream\>. The serialize
       method must have a bool return value.
        @param stream The stream object. May be a read, write or measure stream.
        @param refnum The object's refnum; an int32 and uint32.
        Note that `hasContentFile` is a method, so we're unsure what implications may be associated with just making up
       refNums on the fly.
     */

#define serialize_ref_num(stream, refNum)                                                                              \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!serialize::serialize_ref_num_internal(stream, refNum))                                                    \
        {                                                                                                              \
            return false;                                                                                              \
        }                                                                                                              \
    } while (0)

    // DynamicStats (CreatureStats)

    // ESM3::Skills
    template <typename Stream>
    bool serialize_esm3_skills_internal(Stream& stream, std::map<ESM::RefId, MWMechanics::SkillValue>& skillsMap)
    {
        if (Stream::IsReading)
        {
            uint numSkills;
            serialize_int(stream, numSkills, 0, std::numeric_limits<int>::max());

            for (uint i = 0; i < numSkills; ++i)
            {
                float base, damage, modifier, progress;
                ESM::RefId refId;
                MWMechanics::SkillValue skillValue;
                serialize_ref_id(stream, refId);
                serialize_float(stream, base);
                serialize_float(stream, damage);
                serialize_float(stream, modifier);
                serialize_float(stream, progress);
                skillValue.setBase(base);
                skillValue.setModifier(modifier);
                skillValue.setProgress(progress);
                skillValue.damage(damage);
                skillsMap.insert(std::make_pair(refId, skillValue));
            }
        }

        if (Stream::IsWriting)
        {
            uint numSkills = skillsMap.size();
            serialize_int(stream, numSkills, 0, std::numeric_limits<int>::max());
            for (const auto& pair : skillsMap)
            {
                float base = pair.second.getBase();
                float damage = pair.second.getDamage();
                float modifier = pair.second.getModifier();
                float progress = pair.second.getProgress();
                ESM::RefId refId = pair.first;
                serialize_ref_id(stream, refId);
                serialize_float(stream, base);
                serialize_float(stream, damage);
                serialize_float(stream, modifier);
                serialize_float(stream, progress);
            }
        }

        return true;
    }

    /**
        DREAMWEAVE SERIALIZATION:
        Serialize ESM3::Skills to the stream (read/write/measure).
        This is a helper macro to make writing unified serialize functions easier.
        Serialize macros returns false on error so we don't need to use exceptions for error handling on read. This is
       an important safety measure because packet data comes from the network and may be malicious. IMPORTANT: This
       macro must be called inside a templated serialize function with template \<typename Stream\>. The serialize
       method must have a bool return value.
        @param stream The stream object. May be a read, write or measure stream.
        @param skillsMap The object's skills; a std::map of ESM::RefIds to MWMechanics::SkillValues.
        Note that `hasContentFile` is a method, so we're unsure what implications may be associated with just making up
       refNums on the fly.
     */

#define serialize_esm3_skills(stream, skillsMap)                                                                       \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!serialize::serialize_esm3_skills_internal(stream, skillsMap))                                             \
        {                                                                                                              \
            return false;                                                                                              \
        }                                                                                                              \
    } while (0)
    // ESM::Attribute

    // ESM::Class ?

    // Whatever the spell list is

    // Inventory/ContainerStore

    // BinaryData
    // Lua binary data is internally a string,
    // but we can't treat it as such because they may have \0
    template <typename Stream>
    bool serialize_lua_data_internal(Stream& stream, LuaUtil::BinaryData& luaData)
    {

        uint length;
        if (Stream::IsReading)
        {
            serialize_int(stream, length, 0, MAX_LUADATA_SIZE);
            serialize_assert(length < MAX_LUADATA_SIZE);
            luaData.resize(length);
            serialize_bytes(stream, (uint8_t*)luaData.data(), length);
        }

        if (Stream::IsWriting)
        {
            length = luaData.size();
            serialize_assert(length < MAX_LUADATA_SIZE);
            serialize_int(stream, length, 0, MAX_LUADATA_SIZE);
            serialize_bytes(stream, (uint8_t*)luaData.data(), length);
        }

        return true;
    }

    /**
        DREAMWEAVE SERIALIZATION:
        Serialize LuaUtil::LuaData to the stream (read/write/measure).
        This is a helper macro to make writing unified serialize functions easier.
        Serialize macros returns false on error so we don't need to use exceptions for error handling on read. This is
       an important safety measure because packet data comes from the network and may be malicious. IMPORTANT: This
       macro must be called inside a templated serialize function with template \<typename Stream\>. The serialize
       method must have a bool return value.
        @param stream The stream object. May be a read, write or measure stream.
        @param luaData an std::string, but actually BinaryData.
        The difference is that serialized lua data can contain \0 who knows where.
     */

#define serialize_lua_data(stream, luaData)                                                                            \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!serialize::serialize_lua_data_internal(stream, luaData))                                                  \
        {                                                                                                              \
            return false;                                                                                              \
        }                                                                                                              \
    } while (0)
}
#endif // ESMSERIALIZE_H_
