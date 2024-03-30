#include "nifstream.hpp"

#include <span>

#include "niffile.hpp"

#include "../to_utf8/to_utf8.hpp"

namespace
{

    // Read a range of elements into a dynamic buffer per-element
    // This one should be used if the type cannot be read contiguously
    // (e.g. quaternions)
    template <class T>
    void readRange(Nif::NIFStream& stream, T* dest, size_t size)
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
        Nif::readDynamicBufferOfType(stream, reinterpret_cast<elementType*>(dest), size * numElements);
    }

}

namespace Nif
{

    unsigned int NIFStream::getVersion() const
    {
        return mReader.getVersion();
    }

    unsigned int NIFStream::getUserVersion() const
    {
        return mReader.getUserVersion();
    }

    unsigned int NIFStream::getBethVersion() const
    {
        return mReader.getBethVersion();
    }

    std::string NIFStream::getSizedString(size_t length)
    {
        std::string str(length, '\0');
        mStream->read(str.data(), length);
        if (mStream->bad())
            throw std::runtime_error("Failed to read sized string of " + std::to_string(length) + " chars");
        size_t end = str.find('\0');
        if (end != std::string::npos)
            str.erase(end);
        if (mEncoder)
            str = mEncoder->getUtf8(str, ToUTF8::BufferAllocationPolicy::UseGrowFactor, mBuffer);
        return str;
    }

    void NIFStream::getSizedStrings(std::vector<std::string>& vec, size_t size)
    {
        vec.resize(size);
        for (size_t i = 0; i < vec.size(); i++)
            vec[i] = getSizedString();
    }

    std::string NIFStream::getVersionString()
    {
        std::string result;
        std::getline(*mStream, result);
        if (mStream->bad())
            throw std::runtime_error("Failed to read version string");
        return result;
    }

    std::string NIFStream::getStringPalette()
    {
        size_t size = get<uint32_t>();
        std::string str(size, '\0');
        mStream->read(str.data(), size);
        if (mStream->bad())
            throw std::runtime_error("Failed to read string palette of " + std::to_string(size) + " chars");
        return str;
    }

    template <>
    void NIFStream::read<osg::Vec2f>(osg::Vec2f& vec)
    {
        readBufferOfType(mStream, vec._v);
    }

    template <>
    void NIFStream::read<osg::Vec3f>(osg::Vec3f& vec)
    {
        readBufferOfType(mStream, vec._v);
    }

    template <>
    void NIFStream::read<osg::Vec4f>(osg::Vec4f& vec)
    {
        readBufferOfType(mStream, vec._v);
    }

    template <>
    void NIFStream::read<Matrix3>(Matrix3& mat)
    {
        readBufferOfType<9>(mStream, reinterpret_cast<float*>(&mat.mValues));
    }

    template <>
    void NIFStream::read<osg::Quat>(osg::Quat& quat)
    {
        std::array<float, 4> data;
        readArray(data);
        quat.w() = data[0];
        quat.x() = data[1];
        quat.y() = data[2];
        quat.z() = data[3];
    }

    template <>
    void NIFStream::read<osg::BoundingSpheref>(osg::BoundingSpheref& sphere)
    {
        read(sphere.center());
        read(sphere.radius());
    }

    template <>
    void NIFStream::read<NiTransform>(NiTransform& transform)
    {
        read(transform.mRotation);
        read(transform.mTranslation);
        read(transform.mScale);
    }

    template <>
    void NIFStream::read<NiQuatTransform>(NiQuatTransform& transform)
    {
        read(transform.mTranslation);
        read(transform.mRotation);
        read(transform.mScale);
        if (getVersion() >= generateVersion(10, 1, 0, 110))
            return;
        if (!get<bool>())
            transform.mTranslation = osg::Vec3f();
        if (!get<bool>())
            transform.mRotation = osg::Quat();
        if (!get<bool>())
            transform.mScale = 1.f;
    }

    template <>
    void NIFStream::read<bool>(bool& data)
    {
        if (getVersion() < generateVersion(4, 1, 0, 0))
            data = get<int32_t>() != 0;
        else
            data = get<int8_t>() != 0;
    }

    template <>
    void NIFStream::read<std::string>(std::string& str)
    {
        if (getVersion() < generateVersion(20, 1, 0, 1))
            str = getSizedString();
        else
            str = mReader.getString(get<uint32_t>());
    }

    template <>
    void NIFStream::read<osg::Vec2f>(osg::Vec2f* dest, size_t size)
    {
        readAlignedRange<float, 2>(mStream, dest, size);
    }

    template <>
    void NIFStream::read<osg::Vec3f>(osg::Vec3f* dest, size_t size)
    {
        readAlignedRange<float, 3>(mStream, dest, size);
    }

    template <>
    void NIFStream::read<osg::Vec4f>(osg::Vec4f* dest, size_t size)
    {
        readAlignedRange<float, 4>(mStream, dest, size);
    }

    template <>
    void NIFStream::read<Matrix3>(Matrix3* dest, size_t size)
    {
        readAlignedRange<float, 9>(mStream, dest, size);
    }

    template <>
    void NIFStream::read<osg::Quat>(osg::Quat* dest, size_t size)
    {
        readRange(*this, dest, size);
    }

    template <>
    void NIFStream::read<osg::BoundingSpheref>(osg::BoundingSpheref* dest, size_t size)
    {
        readRange(*this, dest, size);
    }

    template <>
    void NIFStream::read<NiTransform>(NiTransform* dest, size_t size)
    {
        readRange(*this, dest, size);
    }

    template <>
    void NIFStream::read<NiQuatTransform>(NiQuatTransform* dest, size_t size)
    {
        readRange(*this, dest, size);
    }

    template <>
    void NIFStream::read<bool>(bool* dest, size_t size)
    {
        if (getVersion() < generateVersion(4, 1, 0, 0))
        {
            for (bool& value : std::span(dest, size))
                value = get<int32_t>() != 0;
        }
        else
        {
            for (bool& value : std::span(dest, size))
                value = get<int8_t>() != 0;
        }
    }

    template <>
    void NIFStream::read<std::string>(std::string* dest, size_t size)
    {
        if (getVersion() < generateVersion(20, 1, 0, 1))
        {
            for (std::string& value : std::span(dest, size))
                value = getSizedString();
        }
        else
        {
            for (std::string& value : std::span(dest, size))
                value = mReader.getString(get<uint32_t>());
        }
    }

}
