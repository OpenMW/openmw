#include "stream.hpp"

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
        if (mStream->bad())
            throw std::runtime_error("Failed to read sized string of " + std::to_string(length) + " chars");
        std::size_t end = str.find('\0');
        if (end != std::string::npos)
            str.erase(end);
    }
}
