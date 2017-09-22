///Functions used to read raw binary data from .nif files

#ifndef OPENMW_COMPONENTS_NIF_NIFSTREAM_HPP
#define OPENMW_COMPONENTS_NIF_NIFSTREAM_HPP

#include <cassert>
#include <stdint.h>
#include <stdexcept>
#include <vector>

#include <components/files/constrainedfilestream.hpp>

#include <osg/Vec3f>
#include <osg/Vec4f>
#include <osg/Quat>

#include "niftypes.hpp"

namespace Nif
{

class NIFFile;

template <uint32_t numInstances, typename T> inline void readLittleEndianBufferOfType(Files::IStreamPtr &pIStream, T* dest)
{
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386) || defined(_M_IX86)
    pIStream->read((char*)dest, numInstances * sizeof(T));
#else
    char buffer[numInstances * sizeof(T)];
    pIStream->read((char*)buffer, numInstances * sizeof(T));
    /*
        Due to the loop iterations being known at compile time,
        this nested loop will most likely be unrolled
    */
    for (uint32_t i = 0; i < numInstances; i++)
    {
        dest[i] = 0;
        for (uint32_t byte = 0; byte < sizeof(T); byte++)
            dest[i] |= ((T)buffer[i * sizeof(T) + byte]) << (byte * 8);
    }
#endif
}

template<typename type> type inline readLittleEndianType(Files::IStreamPtr &pIStream)
{
    type val;
    readLittleEndianBufferOfType<1,type>(pIStream, (type*)&val);
    return val;
}

class NIFStream {

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

    osg::Vec2f getVector2() {
        osg::Vec2f vec;
        readLittleEndianBufferOfType<2,float>(inp, (float*)&vec._v[0]);
        return vec;
    }
    osg::Vec3f getVector3() {
        osg::Vec3f vec;
        readLittleEndianBufferOfType<3, float>(inp, (float*)&vec._v[0]);
        return vec;
    }
    osg::Vec4f getVector4() {
        osg::Vec4f vec;
        readLittleEndianBufferOfType<4, float>(inp, (float*)&vec._v[0]);
        return vec;
    }
    Matrix3 getMatrix3() {
        Matrix3 mat;
        readLittleEndianBufferOfType<9, float>(inp, (float*)&mat.mValues);
        return mat;
    }
    osg::Quat getQuaternion() {
        float f[4];
        readLittleEndianBufferOfType<4, float>(inp, (float*)&f);
        osg::Quat quat;
        quat.w() = f[0];
        quat.x() = f[1];
        quat.y() = f[2];
        quat.z() = f[3];
        return quat;
    }
    Transformation getTrafo() {
        Transformation t;
        t.pos = getVector3();
        t.rotation = getMatrix3();
        t.scale = getFloat();
        return t;
    }

    ///Read in a string of the given length
    std::string getString(size_t length) {
        std::vector<char> str(length + 1, 0);

        inp->read(&str[0], length);

        return &str[0];
    }
    ///Read in a string of the length specified in the file
    std::string getString() {
        size_t size = readLittleEndianType<uint32_t>(inp);
        return getString(size);
    }
    ///This is special since the version string doesn't start with a number, and ends with "\n"
    std::string getVersionString() {
        std::string result;
        std::getline(*inp, result);
        return result;
    }

    void getUShorts(std::vector<unsigned short> &vec, size_t size) {
        vec.resize(size);
        for (size_t i = 0;i < vec.size();i++)
            vec[i] = getUShort();
    }
    void getFloats(std::vector<float> &vec, size_t size) {
        vec.resize(size);
        for (size_t i = 0;i < vec.size();i++)
            vec[i] = getFloat();
    }
    void getVector2s(std::vector<osg::Vec2f> &vec, size_t size) {
        vec.resize(size);
        for (size_t i = 0;i < vec.size();i++)
            vec[i] = getVector2();
    }
    void getVector3s(std::vector<osg::Vec3f> &vec, size_t size) {
        vec.resize(size);
        for (size_t i = 0;i < vec.size();i++)
            vec[i] = getVector3();
    }
    void getVector4s(std::vector<osg::Vec4f> &vec, size_t size) {
        vec.resize(size);
        for (size_t i = 0;i < vec.size();i++)
            vec[i] = getVector4();
    }
    void getQuaternions(std::vector<osg::Quat> &quat, size_t size) {
        quat.resize(size);
        for (size_t i = 0;i < quat.size();i++)
            quat[i] = getQuaternion();
    }
};

}

#endif
