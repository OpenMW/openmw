///Functions used to read raw binary data from .nif files

#ifndef OPENMW_COMPONENTS_NIF_NIFSTREAM_HPP
#define OPENMW_COMPONENTS_NIF_NIFSTREAM_HPP
#include <xmmintrin.h>
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

class NIFStream {

    /// Input stream
    Files::IStreamPtr inp;

    uint8_t read_byte() {
        uint8_t byte;
        inp->read((char*)&byte, 1);
        return byte;
    }

    uint16_t read_le16() {
        alignas(2) uint8_t buffer[2];
        inp->read((char*)buffer, 2);
        return static_cast<uint16_t>(*((uint16_t*)buffer));
    }
    uint32_t read_le32() {
        alignas(4) uint8_t buffer[4];
        inp->read((char*)buffer, 4);
        return static_cast<uint32_t>(*((uint32_t*)buffer));
    }
    uint64_t read_le64() {
        alignas(8) uint8_t buffer[8];
        inp->read((char*)buffer, 8);
        return static_cast<uint64_t>(*((uint64_t*)buffer));
    }
    __m128 read_le96() {
        alignas(8) uint8_t buffer[16];
        inp->read((char*)buffer, 12);
        return static_cast<__m128>(*((__m128*)buffer));
    }
    __m128 read_le128() {
        alignas(16) uint8_t buffer[16];
        inp->read((char*)buffer, 16);
        return static_cast<__m128>(*((__m128*)buffer));
    }
    float read_le32f() {
        union {
            uint32_t i;
            float f;
        } u = { read_le32() };
        return u.f;
    }

public:

    NIFFile * const file;

    NIFStream (NIFFile * file, Files::IStreamPtr inp): inp (inp), file (file) {}

    void skip(size_t size) { inp->ignore(size); }

    char getChar() { return read_byte(); }
    short getShort() { return read_le16(); }
    unsigned short getUShort() { return read_le16(); }
    int getInt() { return read_le32(); }
    unsigned int getUInt() { return read_le32(); }
    float getFloat() { return read_le32f(); }

    osg::Vec2f getVector2() {
        union {
            uint64_t i;
            float f[2];
        } u = { read_le64() };
        osg::Vec2f vec;
        for (size_t i = 0;i < 2;i++)
            vec._v[i] = u.f[i];
        return vec;
    }
    osg::Vec3f getVector3() {
        union {
            __m128 i;
            float f[4];
        } u = { read_le96() };
        osg::Vec3f vec;
        for (size_t i = 0;i < 3;i++)
            vec._v[i] = u.f[i];
        return vec;
    }
    osg::Vec4f getVector4() {
        union {
            __m128 i;
            float f[4];
        } u = { read_le128() };
        osg::Vec4f vec;
        for (size_t i = 0;i < 4;i++)
            vec._v[i] = u.f[i];
        return vec;
    }
    Matrix3 getMatrix3() {
        Matrix3 mat;
        alignas(16) union {
            float f[9];
            uint8_t buffer[36];
        } u;
        inp->read((char*)u.buffer, 36);
        for (size_t i = 0;i < 3;i++)
        {
            for (size_t j = 0;j < 3;j++)
                mat.mValues[i][j] = u.f[3*i+j];
        }
        return mat;
    }
    osg::Quat getQuaternion() {
        union {
            __m128 i;
            float f[4];
        } u = { read_le128() };
        osg::Quat quat;
        quat.w() = u.f[0];
        quat.x() = u.f[1];
        quat.y() = u.f[2];
        quat.z() = u.f[3];
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
        size_t size = read_le32();
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
