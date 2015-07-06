///Functions used to read raw binary data from .nif files

#ifndef OPENMW_COMPONENTS_NIF_NIFSTREAM_HPP
#define OPENMW_COMPONENTS_NIF_NIFSTREAM_HPP

#include <stdint.h>
#include <stdexcept>
#include <vector>

#include <components/files/constrainedfilestream.hpp>

#include <osg/Vec3f>
#include <osg/Vec4f>
#include <osg/Quat>
#include <osg/Array>
#include <osg/PrimitiveSet>

#include "niftypes.hpp"


namespace Nif
{

class NIFFile;

class NIFStream {

    /// Input stream
    Files::IStreamPtr inp;

    uint8_t read_byte();
    uint16_t read_le16();
    uint32_t read_le32();
    float read_le32f();

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

    osg::Vec2f getVector2();
    osg::Vec3f getVector3();
    osg::Vec4f getVector4();
    Matrix3 getMatrix3();
    osg::Quat getQuaternion();
    Transformation getTrafo();

    ///Read in a string of the given length
    std::string getString(size_t length);
    ///Read in a string of the length specified in the file
    std::string getString();
    ///This is special since the version string doesn't start with a number, and ends with "\n"
    std::string getVersionString();

    void getUShorts(osg::VectorGLushort* vec, size_t size);
    void getFloats(std::vector<float> &vec, size_t size);
    void getVector2s(osg::Vec2Array* vec, size_t size);
    void getVector3s(osg::Vec3Array* vec, size_t size);
    void getVector4s(osg::Vec4Array* vec, size_t size);
    void getQuaternions(std::vector<osg::Quat> &quat, size_t size);
};

}

#endif
