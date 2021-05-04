///Functions used to read raw binary data from .nif files

#ifndef OPENMW_COMPONENTS_NIF_NIFSTREAM_HPP
#define OPENMW_COMPONENTS_NIF_NIFSTREAM_HPP

#include <cassert>
#include <stdint.h>
#include <stdexcept>
#include <vector>
#include <typeinfo>
#include <type_traits>

#include <components/files/constrainedfilestream.hpp>
#include <components/misc/endianness.hpp>

#include <osg/Vec3f>
#include <osg/Vec4f>
#include <osg/Quat>

#include "niftypes.hpp"

namespace Nif
{

class NIFFile;

template <std::size_t numInstances, typename T> inline void readLittleEndianBufferOfType(Files::IStreamPtr &pIStream, T* dest)
{
    static_assert(std::is_arithmetic_v<T>, "Buffer element type is not arithmetic");
    pIStream->read((char*)dest, numInstances * sizeof(T));
    if (pIStream->bad())
        throw std::runtime_error("Failed to read little endian typed (" + std::string(typeid(T).name()) + ") buffer of "
                                 + std::to_string(numInstances) + " instances");
    if constexpr (Misc::IS_BIG_ENDIAN)
        for (std::size_t i = 0; i < numInstances; i++)
            Misc::swapEndiannessInplace(dest[i]);
}

template <typename T> inline void readLittleEndianDynamicBufferOfType(Files::IStreamPtr &pIStream, T* dest, std::size_t numInstances)
{
    static_assert(std::is_arithmetic_v<T>, "Buffer element type is not arithmetic");
    pIStream->read((char*)dest, numInstances * sizeof(T));
    if (pIStream->bad())
        throw std::runtime_error("Failed to read little endian dynamic buffer of " + std::to_string(numInstances) + " instances");
    if constexpr (Misc::IS_BIG_ENDIAN)
        for (std::size_t i = 0; i < numInstances; i++)
            Misc::swapEndiannessInplace(dest[i]);
}
template<typename type> type inline readLittleEndianType(Files::IStreamPtr &pIStream)
{
    type val;
    readLittleEndianBufferOfType<1, type>(pIStream, &val);
    return val;
}

class NIFStream
{
    /// Input stream
    Files::IStreamPtr inp;

public:

    NIFFile * const file;

    NIFStream (NIFFile * file, Files::IStreamPtr inp): inp (inp), file (file) {}

    void skip(size_t size) { inp->ignore(size); }

    char getChar()
    {
        return readLittleEndianType<char>(inp);
    }

    short getShort()
    {
        return readLittleEndianType<short>(inp);
    }

    unsigned short getUShort()
    {
        return readLittleEndianType<unsigned short>(inp);
    }

    int getInt()
    {
        return readLittleEndianType<int>(inp);
    }

    unsigned int getUInt()
    {
        return readLittleEndianType<unsigned int>(inp);
    }

    float getFloat()
    {
        return readLittleEndianType<float>(inp);
    }

    osg::Vec2f getVector2()
    {
        osg::Vec2f vec;
        readLittleEndianBufferOfType<2,float>(inp, vec._v);
        return vec;
    }

    osg::Vec3f getVector3()
    {
        osg::Vec3f vec;
        readLittleEndianBufferOfType<3, float>(inp, vec._v);
        return vec;
    }

    osg::Vec4f getVector4()
    {
        osg::Vec4f vec;
        readLittleEndianBufferOfType<4, float>(inp, vec._v);
        return vec;
    }

    Matrix3 getMatrix3()
    {
        Matrix3 mat;
        readLittleEndianBufferOfType<9, float>(inp, (float*)&mat.mValues);
        return mat;
    }

    osg::Quat getQuaternion();

    Transformation getTrafo();

    bool getBoolean();

    std::string getString();

    unsigned int getVersion() const;
    unsigned int getUserVersion() const;
    unsigned int getBethVersion() const;

    // Convert human-readable version numbers into a number that can be compared.
    static constexpr uint32_t generateVersion(uint8_t major, uint8_t minor, uint8_t patch, uint8_t rev)
    {
        return (major << 24) + (minor << 16) + (patch << 8) + rev;
    }

    ///Read in a string of the given length
    std::string getSizedString(size_t length)
    {
        std::string str(length, '\0');
        inp->read(str.data(), length);
        if (inp->bad())
            throw std::runtime_error("Failed to read sized string of " + std::to_string(length) + " chars");
        return str;
    }
    ///Read in a string of the length specified in the file
    std::string getSizedString()
    {
        size_t size = readLittleEndianType<uint32_t>(inp);
        return getSizedString(size);
    }

    ///Specific to Bethesda headers, uses a byte for length
    std::string getExportString()
    {
        size_t size = static_cast<size_t>(readLittleEndianType<uint8_t>(inp));
        return getSizedString(size);
    }

    ///This is special since the version string doesn't start with a number, and ends with "\n"
    std::string getVersionString()
    {
        std::string result;
        std::getline(*inp, result);
        if (inp->bad())
            throw std::runtime_error("Failed to read version string");
        return result;
    }

    void getChars(std::vector<char> &vec, size_t size)
    {
        vec.resize(size);
        readLittleEndianDynamicBufferOfType<char>(inp, vec.data(), size);
    }

    void getUChars(std::vector<unsigned char> &vec, size_t size)
    {
        vec.resize(size);
        readLittleEndianDynamicBufferOfType<unsigned char>(inp, vec.data(), size);
    }

    void getUShorts(std::vector<unsigned short> &vec, size_t size)
    {
        vec.resize(size);
        readLittleEndianDynamicBufferOfType<unsigned short>(inp, vec.data(), size);
    }

    void getFloats(std::vector<float> &vec, size_t size)
    {
        vec.resize(size);
        readLittleEndianDynamicBufferOfType<float>(inp, vec.data(), size);
    }

    void getInts(std::vector<int> &vec, size_t size)
    {
        vec.resize(size);
        readLittleEndianDynamicBufferOfType<int>(inp, vec.data(), size);
    }

    void getUInts(std::vector<unsigned int> &vec, size_t size)
    {
        vec.resize(size);
        readLittleEndianDynamicBufferOfType<unsigned int>(inp, vec.data(), size);
    }

    void getVector2s(std::vector<osg::Vec2f> &vec, size_t size)
    {
        vec.resize(size);
        /* The packed storage of each Vec2f is 2 floats exactly */
        readLittleEndianDynamicBufferOfType<float>(inp,(float*)vec.data(), size*2);
    }

    void getVector3s(std::vector<osg::Vec3f> &vec, size_t size)
    {
        vec.resize(size);
        /* The packed storage of each Vec3f is 3 floats exactly */
        readLittleEndianDynamicBufferOfType<float>(inp, (float*)vec.data(), size*3);
    }

    void getVector4s(std::vector<osg::Vec4f> &vec, size_t size)
    {
        vec.resize(size);
        /* The packed storage of each Vec4f is 4 floats exactly */
        readLittleEndianDynamicBufferOfType<float>(inp, (float*)vec.data(), size*4);
    }

    void getQuaternions(std::vector<osg::Quat> &quat, size_t size)
    {
        quat.resize(size);
        for (size_t i = 0;i < quat.size();i++)
            quat[i] = getQuaternion();
    }

    void getStrings(std::vector<std::string> &vec, size_t size)
    {
        vec.resize(size);
        for (size_t i = 0; i < vec.size(); i++)
            vec[i] = getString();
    }
    /// We need to use this when the string table isn't actually initialized.
    void getSizedStrings(std::vector<std::string> &vec, size_t size)
    {
        vec.resize(size);
        for (size_t i = 0; i < vec.size(); i++)
            vec[i] = getSizedString();
    }
};

}

#endif
