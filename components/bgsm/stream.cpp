#include "stream.hpp"

#include <span>

#include "reader.hpp"

namespace
{

    // Read a range of elements into a dynamic buffer per-element
    // This one should be used if the type cannot be read contiguously
    // (e.g. quaternions)
    template <class T>
    void readRange(Bgsm::BGSMStream& stream, T* dest, size_t size)
    {
        for (T& value : std::span(dest, size))
            stream.read(value);
    }

    // Read a range of elements into a dynamic buffer
    // This one should be used if the type can be read contiguously as an array of a different type
    // (e.g. osg::VecXf can be read as a float array of X elements)
    template <class elementType, size_t numElements, class T>
    void readAlignedRange(Files::IStreamPtr& stream, T* dest, size_t size)
    {
        static_assert(std::is_standard_layout_v<T>);
        static_assert(std::alignment_of_v<T> == std::alignment_of_v<elementType>);
        static_assert(sizeof(T) == sizeof(elementType) * numElements);
        Bgsm::readDynamicBufferOfType(stream, reinterpret_cast<elementType*>(dest), size * numElements);
    }

}

namespace Bgsm
{

    std::uint32_t BGSMStream::getVersion() const
    {
        return mReader.getVersion();
    }

    std::string BGSMStream::getSizedString(size_t length)
    {
        // Prevent potential memory allocation freezes; strings this long are not expected in BGSM
        if (length > 1024)
            throw std::runtime_error("Requested string length is too large: " + std::to_string(length));
        std::string str(length, '\0');
        mStream->read(str.data(), length);
        if (mStream->bad())
            throw std::runtime_error("Failed to read sized string of " + std::to_string(length) + " chars");
        size_t end = str.find('\0');
        if (end != std::string::npos)
            str.erase(end);
        return str;
    }

    void BGSMStream::getSizedStrings(std::vector<std::string>& vec, size_t size)
    {
        vec.resize(size);
        for (size_t i = 0; i < vec.size(); i++)
            vec[i] = getSizedString();
    }

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
        str = getSizedString();
    }

    template <>
    void BGSMStream::read<osg::Vec2f>(osg::Vec2f* dest, size_t size)
    {
        readAlignedRange<float, 2>(mStream, dest, size);
    }

    template <>
    void BGSMStream::read<osg::Vec3f>(osg::Vec3f* dest, size_t size)
    {
        readAlignedRange<float, 3>(mStream, dest, size);
    }

    template <>
    void BGSMStream::read<osg::Vec4f>(osg::Vec4f* dest, size_t size)
    {
        readAlignedRange<float, 4>(mStream, dest, size);
    }

    template <>
    void BGSMStream::read<std::string>(std::string* dest, size_t size)
    {
        for (std::string& value : std::span(dest, size))
            value = getSizedString();
    }

}
