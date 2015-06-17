#include "nifstream.hpp"
//For error reporting
#include "niffile.hpp"

namespace Nif
{

//Private functions
uint8_t NIFStream::read_byte()
{
    uint8_t byte;
    inp->read((char*)&byte, 1);
    return byte;
}
uint16_t NIFStream::read_le16()
{
    uint8_t buffer[2];
    inp->read((char*)buffer, 2);
    return buffer[0] | (buffer[1]<<8);
}
uint32_t NIFStream::read_le32()
{
    uint8_t buffer[4];
    inp->read((char*)buffer, 4);
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
osg::Vec2f NIFStream::getVector2()
{
    osg::Vec2f vec;
    for(size_t i = 0;i < 2;i++)
        vec._v[i] = getFloat();
    return vec;
}
osg::Vec3f NIFStream::getVector3()
{
    osg::Vec3f vec;
    for(size_t i = 0;i < 3;i++)
        vec._v[i] = getFloat();
    return vec;
}
osg::Vec4f NIFStream::getVector4()
{
    osg::Vec4f vec;
    for(size_t i = 0;i < 4;i++)
        vec._v[i] = getFloat();
    return vec;
}
Matrix3 NIFStream::getMatrix3()
{
    Matrix3 mat;
    for(size_t i = 0;i < 3;i++)
    {
        for(size_t j = 0;j < 3;j++)
            mat.mValues[i][j] = getFloat();
    }
    return mat;
}
osg::Quat NIFStream::getQuaternion()
{
    osg::Quat quat;
    quat.w() = getFloat();
    quat.x() = getFloat();
    quat.y() = getFloat();
    quat.z() = getFloat();
    return quat;
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
    std::vector<char> str (length+1, 0);

    inp->read(&str[0], length);

    return &str[0];
}
std::string NIFStream::getString()
{
    size_t size = read_le32();
    return getString(size);
}
std::string NIFStream::getVersionString()
{
    std::string result;
    std::getline(*inp, result);
    return result;
}

void NIFStream::getUShorts(osg::VectorGLushort* vec, size_t size)
{
    vec->reserve(size);
    for(size_t i = 0;i < size;i++)
        vec->push_back(getUShort());
}
void NIFStream::getFloats(std::vector<float> &vec, size_t size)
{
    vec.resize(size);
    for(size_t i = 0;i < vec.size();i++)
        vec[i] = getFloat();
}
void NIFStream::getVector2s(osg::Vec2Array* vec, size_t size)
{
    vec->reserve(size);
    for(size_t i = 0;i < size;i++)
        vec->push_back(getVector2());
}
void NIFStream::getVector3s(osg::Vec3Array* vec, size_t size)
{
    vec->reserve(size);
    for(size_t i = 0;i < size;i++)
        vec->push_back(getVector3());
}
void NIFStream::getVector4s(osg::Vec4Array* vec, size_t size)
{
    vec->reserve(size);
    for(size_t i = 0;i < size;i++)
        vec->push_back(getVector4());
}
void NIFStream::getQuaternions(std::vector<osg::Quat> &quat, size_t size)
{
    quat.resize(size);
    for(size_t i = 0;i < quat.size();i++)
        quat[i] = getQuaternion();
}

}
