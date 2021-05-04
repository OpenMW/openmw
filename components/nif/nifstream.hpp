///Functions used to read raw binary data from .nif files

#ifndef OPENMW_COMPONENTS_NIF_NIFSTREAM_HPP
#define OPENMW_COMPONENTS_NIF_NIFSTREAM_HPP

#include <cassert>
#include <stdint.h>
#include <stdexcept>
#include <vector>

#include <components/files/constrainedfilestream.hpp>
#include <components/misc/endianness.hpp>

#include <osg/Vec3f>
#include <osg/Vec4f>
#include <osg/Quat>

#include "niftypes.hpp"

namespace Nif
{

class NIFFile;

/* 
    readLittleEndianBufferOfType: This template should only be used with arithmetic types
*/
template <uint32_t numInstances, typename T> inline void readLittleEndianBufferOfType(Files::IStreamPtr &pIStream, T* dest)
{
    pIStream->read((char*)dest, numInstances * sizeof(T));
    if constexpr (Misc::IS_BIG_ENDIAN)
        for (uint32_t i = 0; i < numInstances; i++)
            Misc::swapEndiannessInplace(dest[i]);
}

/*
    readLittleEndianDynamicBufferOfType: This template should only be used with arithmetic types
*/
template <typename T> inline void readLittleEndianDynamicBufferOfType(Files::IStreamPtr &pIStream, T* dest, uint32_t numInstances)
{
    pIStream->read((char*)dest, numInstances * sizeof(T));
    if constexpr (Misc::IS_BIG_ENDIAN)
        for (uint32_t i = 0; i < numInstances; i++)
            Misc::swapEndiannessInplace(dest[i]);
}
template<typename type> type inline readLittleEndianType(Files::IStreamPtr &pIStream)
{
    type val;
    readLittleEndianBufferOfType<1, type>(pIStream, (type*)&val);
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
        readLittleEndianBufferOfType<2,float>(inp, (float*)&vec._v[0]);
        return vec;
    }

    osg::Vec3f getVector3()
    {
        osg::Vec3f vec;
        readLittleEndianBufferOfType<3, float>(inp, (float*)&vec._v[0]);
        return vec;
    }

    osg::Vec4f getVector4()
    {
        osg::Vec4f vec;
        readLittleEndianBufferOfType<4, float>(inp, (float*)&vec._v[0]);
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
        std::vector<char> str(length + 1, 0);

        inp->read(str.data(), length);

        return str.data();
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
