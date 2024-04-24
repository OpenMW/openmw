#ifndef ESMSERIALIZE_H_
#define ESMSERIALIZE_H_

#include <osg/Vec3f>
#include <serialize.h>

#include <components/esm/position.hpp>
#include <components/esm/refid.hpp>

constexpr std::size_t MAX_STRING_LENGTH = 256 + sizeof(int) + 1;

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
        Serialize an std::string to the stream (read/write/measure).
        This is a helper macro to make writing unified serialize functions easier.
        Serialize macros returns false on error so we don't need to use exceptions for error handling on read. This is
       an important safety measure because packet data comes from the network and may be malicious. IMPORTANT: This
       macro must be called inside a templated serialize function with template \<typename Stream\>. The serialize
       method must have a bool return value.
        @param stream The stream object. May be a read, write or measure stream.
        @param osg::Vec3f Vector to serialize to.
       buffer.
     */

#define serialize_vec3f(stream, string)                                                                                \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!serialize::serialize_vec3f_internal(stream, string))                                                      \
        {                                                                                                              \
            return false;                                                                                              \
        }                                                                                                              \
    } while (0)

    // ESM::Position
    template <typename Stream>
    bool serialize_esm_position_internal(Stream& stream, ESM::Position& position)
    {
        serialize_vec3f_internal(stream, position.pos);
        serialize_vec3f_internal(stream, position.rot);
        return true;
    }

    /**
        DREAMWEAVE SERIALIZATION:
        Serialize an std::string to the stream (read/write/measure).
        This is a helper macro to make writing unified serialize functions easier.
        Serialize macros returns false on error so we don't need to use exceptions for error handling on read. This is
       an important safety measure because packet data comes from the network and may be malicious. IMPORTANT: This
       macro must be called inside a templated serialize function with template \<typename Stream\>. The serialize
       method must have a bool return value.
        @param stream The stream object. May be a read, write or measure stream.
        @param esm::position The object position. Note that esm::position is composed of position and rotation.
     */

#define serialize_esm_position(stream, position)                                                                       \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!serialize::serialize_esm_position_internal(stream, string))                                               \
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
        Serialize an std::string to the stream (read/write/measure).
        This is a helper macro to make writing unified serialize functions easier.
        Serialize macros returns false on error so we don't need to use exceptions for error handling on read. This is
       an important safety measure because packet data comes from the network and may be malicious. IMPORTANT: This
       macro must be called inside a templated serialize function with template \<typename Stream\>. The serialize
       method must have a bool return value.
        @param stream The stream object. May be a read, write or measure stream.
        @param esm::refid The object's id. Internally this serializer uses the toString method of refId.
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
    // DynamicStats
    // ESM::Skill
    // ESM::Attribute
    // ESM::Class ?
    // Whatever the spell list is

    // Inventory/ContainerStore
}
#endif // ESMSERIALIZE_H_
