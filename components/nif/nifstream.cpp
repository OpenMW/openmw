#include "nifstream.hpp"
//For error reporting
#include "niffile.hpp"

namespace Nif
{

//Private functions
uint8_t NIFStream::read_byte()
{
    uint8_t byte;
    if(inp->read(&byte, 1) != 1) return 0;
    return byte;
}
uint16_t NIFStream::read_le16()
{
    uint8_t buffer[2];
    if(inp->read(buffer, 2) != 2) return 0;
    return buffer[0] | (buffer[1]<<8);
}
uint32_t NIFStream::read_le32()
{
    uint8_t buffer[4];
    if(inp->read(buffer, 4) != 4) return 0;
    return buffer[0] | (buffer[1]<<8) | (buffer[2]<<16) | (buffer[3]<<24);
}
float NIFStream::read_le32f()
{
    union {
        uint32_t i;
        float f;
    } u = { read_le32() };
    return u.f;
}

//Public functions
Ogre::Vector2 NIFStream::getVector2()
{
    float a[2];
    for(size_t i = 0;i < 2;i++)
        a[i] = getFloat();
    return Ogre::Vector2(a);
}
Ogre::Vector3 NIFStream::getVector3()
{
    float a[3];
    for(size_t i = 0;i < 3;i++)
        a[i] = getFloat();
    return Ogre::Vector3(a);
}
Ogre::Vector4 NIFStream::getVector4()
{
    float a[4];
    for(size_t i = 0;i < 4;i++)
        a[i] = getFloat();
    return Ogre::Vector4(a);
}
Ogre::Matrix3 NIFStream::getMatrix3()
{
    Ogre::Real a[3][3];
    for(size_t i = 0;i < 3;i++)
    {
        for(size_t j = 0;j < 3;j++)
            a[i][j] = Ogre::Real(getFloat());
    }
    return Ogre::Matrix3(a);
}
Ogre::Quaternion NIFStream::getQuaternion()
{
    float a[4];
    for(size_t i = 0;i < 4;i++)
        a[i] = getFloat();
    return Ogre::Quaternion(a);
}
Transformation NIFStream::getTrafo()
{
    Transformation t;
    t.pos = getVector3();
    t.rotation = getMatrix3();
    t.scale = getFloat();
    return t;
}

std::string NIFStream::getString(size_t length)
{
    //Make sure we're not reading in too large of a string
    unsigned int fileSize = inp->size();
    if(fileSize != 0 && fileSize < length)
        file->fail("Attempted to read a string with " + Ogre::StringConverter::toString(length) + " characters , but file is only "+Ogre::StringConverter::toString(fileSize)+ " bytes!");

    std::vector<char> str (length+1, 0);

    if(inp->read(&str[0], length) != length)
        throw std::runtime_error (":  String length in NIF file "+ file->getFilename() +" does not match!  Expected length:  "
            + Ogre::StringConverter::toString(length));

    return &str[0];
}
std::string NIFStream::getString()
{
    size_t size = read_le32();
    return getString(size);
}
std::string NIFStream::getVersionString()
{
    return inp->getLine();
}

template <>
char NIFStream::get<char>(){ return getChar(); }
template <>
short NIFStream::get<short>(){ return getShort(); }
template <>
unsigned short NIFStream::get<unsigned short>(){ return getUShort(); }
template <>
int NIFStream::get<int>(){ return getInt(); }
template <>
unsigned int NIFStream::get<unsigned int>(){ return getUInt(); }
template <>
float NIFStream::get<float>(){ return getFloat(); }

template <>
Ogre::Vector2 NIFStream::get<Ogre::Vector2>(){ return getVector2(); }
template <>
Ogre::Vector3 NIFStream::get<Ogre::Vector3>(){ return getVector3(); }
template <>
Ogre::Vector4 NIFStream::get<Ogre::Vector4>(){ return getVector4(); }
template <>
Ogre::Matrix3 NIFStream::get<Ogre::Matrix3>(){ return getMatrix3(); }
template <>
Ogre::Quaternion NIFStream::get<Ogre::Quaternion>(){ return getQuaternion(); }
template <>
Transformation NIFStream::get<Transformation>(){ return getTrafo(); }

template <>
std::string NIFStream::get<std::string>(){ return getString(); }

}
