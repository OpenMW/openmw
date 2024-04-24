#ifndef ESMSERIALIZE_H_
#define ESMSERIALIZE_H_

#include <osg/Vec3f>
#include <serialize.h>

#include <components/esm/position.hpp>

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

#endif // ESMSERIALIZE_H_
