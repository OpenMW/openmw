///Functions used to read raw binary data from .nif files

#ifndef OPENMW_COMPONENTS_NIF_NIFSTREAM_HPP
#define OPENMW_COMPONENTS_NIF_NIFSTREAM_HPP

#include <stdint.h>
#include <stdexcept>

#include <OgreDataStream.h>
#include <OgreVector2.h>
#include <OgreVector3.h>
#include <OgreVector4.h>
#include <OgreMatrix3.h>
#include <OgreQuaternion.h>
#include <OgreStringConverter.h>

#include "niftypes.hpp"

namespace Nif
{

class NIFFile;

class NIFStream {

    /// Input stream
    Ogre::DataStreamPtr inp;

    uint8_t read_byte();
    uint16_t read_le16();
    uint32_t read_le32();
    float read_le32f();

public:

    NIFFile * const file;

    NIFStream (NIFFile * file, Ogre::DataStreamPtr inp): inp (inp), file (file) {}

    void skip(size_t size) { inp->skip(size); }

    char getChar() { return read_byte(); }
    short getShort() { return read_le16(); }
    unsigned short getUShort() { return read_le16(); }
    int getInt() { return read_le32(); }
    unsigned int getUInt() { return read_le32(); }
    float getFloat() { return read_le32f(); }

    Ogre::Vector2 getVector2();
    Ogre::Vector3 getVector3();
    Ogre::Vector4 getVector4();
    Ogre::Matrix3 getMatrix3();
    Ogre::Quaternion getQuaternion();
    Transformation getTrafo();

    ///Read in a string of the given length
    std::string getString(size_t length);
    ///Read in a string of the length specified in the file
    std::string getString();
    ///This is special since the version string doesn't start with a number, and ends with "\n"
    std::string getVersionString();

    void getShorts(std::vector<short> &vec, size_t size);
    void getFloats(std::vector<float> &vec, size_t size);
    void getVector2s(std::vector<Ogre::Vector2> &vec, size_t size);
    void getVector3s(std::vector<Ogre::Vector3> &vec, size_t size);
    void getVector4s(std::vector<Ogre::Vector4> &vec, size_t size);
    void getQuaternions(std::vector<Ogre::Quaternion> &quat, size_t size);
};

}

#endif
