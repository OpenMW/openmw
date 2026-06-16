#include "stream.hpp"

#include <cstdint>
#include <format>
#include <istream>
#include <stdexcept>
#include <type_traits>
#include <string>

#include <components/misc/endianness.hpp>

namespace Spt
{
    namespace
    {
        template <typename T>
        void read(Files::IStreamPtr& stream, T& value)
        {
            static_assert(std::is_trivially_copyable_v<T>);
            stream->read(reinterpret_cast<char*>(&value), sizeof(T));
            if (stream->fail())
                throw std::runtime_error(
                    std::format("Failed to read {}: {}", typeid(T).name(), std::generic_category().message(errno)));
            if constexpr (Misc::IS_BIG_ENDIAN && sizeof(T) > 1)
                Misc::swapEndiannessInplace(value);
        }
    }

    SPTEOFError::SPTEOFError()
        : std::runtime_error("SPT stream EOF")
    {
    }

    SPTStream::SPTStream(Files::IStreamPtr&& stream)
        : mStream(std::move(stream))
    {
    }

    void SPTStream::skip(size_t size)
    {
        mStream->ignore(size);
    }

    int32_t SPTStream::readInt()
    {
        int32_t value;
        read(mStream, value);
        return value;
    }

    float SPTStream::readFloat()
    {
        float value;
        read(mStream, value);
        return value;
    }

    int8_t SPTStream::readByte()
    {
        int8_t value;
        read(mStream, value);
        return value;
    }

    int16_t SPTStream::readShort()
    {
        int16_t value;
        read(mStream, value);
        return value;
    }

    void SPTStream::readVec2f(osg::Vec2f& vec)
    {
        read(mStream, vec._v[0]);
        read(mStream, vec._v[1]);
    }

    void SPTStream::readVec3f(osg::Vec3f& vec)
    {
        read(mStream, vec._v[0]);
        read(mStream, vec._v[1]);
        read(mStream, vec._v[2]);
    }

    void SPTStream::readVec4f(osg::Vec4f& vec)
    {
        read(mStream, vec._v[0]);
        read(mStream, vec._v[1]);
        read(mStream, vec._v[2]);
        read(mStream, vec._v[3]);
    }

    int32_t SPTStream::readSection()
    {
        if (mStream->peek() == std::char_traits<char>::eof())
            throw SPTEOFError();
        return readInt();
    }

    std::string SPTStream::readIntString()
    {
        int32_t length = readInt();
        if (length > 4096)
            throw std::runtime_error("SPT intstring too long: " + std::to_string(length));
        std::string str(length, '\0');
        mStream->read(str.data(), length);
        if (mStream->fail())
            throw std::runtime_error(std::format(
                "Failed to read intstring of {} chars: {}", length, std::generic_category().message(errno)));
        return str;
    }
}
