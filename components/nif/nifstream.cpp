#include "nifstream.hpp"

#include "niffile.hpp"

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
        readBufferOfType<2>(mStream, vec._v);
    }

    template <>
    void NIFStream::read<osg::Vec3f>(osg::Vec3f& vec)
    {
        readBufferOfType<3>(mStream, vec._v);
    }

    template <>
    void NIFStream::read<osg::Vec4f>(osg::Vec4f& vec)
    {
        readBufferOfType<4>(mStream, vec._v);
    }

    template <>
    void NIFStream::read<Matrix3>(Matrix3& mat)
    {
        readBufferOfType<9>(mStream, (float*)&mat.mValues);
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
    void NIFStream::read<Transformation>(Transformation& t)
    {
        read(t.pos);
        read(t.rotation);
        read(t.scale);
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
        // The packed storage of each Vec2f is 2 floats exactly
        readDynamicBufferOfType<float>(mStream, (float*)dest, size * 2);
    }

    template <>
    void NIFStream::read<osg::Vec3f>(osg::Vec3f* dest, size_t size)
    {
        // The packed storage of each Vec3f is 3 floats exactly
        readDynamicBufferOfType<float>(mStream, (float*)dest, size * 3);
    }

    template <>
    void NIFStream::read<osg::Vec4f>(osg::Vec4f* dest, size_t size)
    {
        // The packed storage of each Vec4f is 4 floats exactly
        readDynamicBufferOfType<float>(mStream, (float*)dest, size * 4);
    }

    template <>
    void NIFStream::read<Matrix3>(Matrix3* dest, size_t size)
    {
        // The packed storage of each Matrix3 is 9 floats exactly
        readDynamicBufferOfType<float>(mStream, (float*)dest, size * 9);
    }

    template <>
    void NIFStream::read<osg::Quat>(osg::Quat* dest, size_t size)
    {
        for (size_t i = 0; i < size; i++)
            read(dest[i]);
    }

    template <>
    void NIFStream::read<Transformation>(Transformation* dest, size_t size)
    {
        for (size_t i = 0; i < size; i++)
            read(dest[i]);
    }

    template <>
    void NIFStream::read<bool>(bool* dest, size_t size)
    {
        if (getVersion() < generateVersion(4, 1, 0, 0))
        {
            for (size_t i = 0; i < size; i++)
                dest[i] = get<int32_t>() != 0;
        }
        else
        {
            for (size_t i = 0; i < size; i++)
                dest[i] = get<int8_t>() != 0;
        }
    }

    template <>
    void NIFStream::read<std::string>(std::string* dest, size_t size)
    {
        if (getVersion() < generateVersion(20, 1, 0, 1))
        {
            for (size_t i = 0; i < size; i++)
                dest[i] = getSizedString();
        }
        else
        {
            for (size_t i = 0; i < size; i++)
                dest[i] = mReader.getString(get<uint32_t>());
        }
    }

}
