#include "stream.hpp"

#include <osg/Vec2f>
#include <osg/Vec3f>
#include <osg/Vec4f>

#include <format>
#include <stdexcept>
#include <string>

namespace Bgsm
{
    template <>
    void BGSMStream::read<osg::Vec2f>(osg::Vec2f& vec)
    {
        readBufferOfType(mStream, vec._v);
    }

    template <>
    void BGSMStream::read<osg::Vec3f>(osg::Vec3f& vec)
    {
        readBufferOfType(mStream, vec._v);
    }

    template <>
    void BGSMStream::read<osg::Vec4f>(osg::Vec4f& vec)
    {
        readBufferOfType(mStream, vec._v);
    }

    template <>
    void BGSMStream::read<std::string>(std::string& str)
    {
        std::uint32_t length;
        read(length);
        // Prevent potential memory allocation freezes; strings this long are not expected in BGSM
        if (length > 1024)
            throw std::runtime_error("Requested string length is too large: " + std::to_string(length));
        str = std::string(length, '\0');
        mStream->read(str.data(), length);
        if (mStream->fail())
            throw std::runtime_error(std::format(
                "Failed to read sized string of {} chars: {}", length, std::generic_category().message(errno)));
        std::size_t end = str.find('\0');
        if (end != std::string::npos)
            str.erase(end);
    }
}
