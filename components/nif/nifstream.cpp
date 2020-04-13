#include "nifstream.hpp"
//For error reporting
#include "niffile.hpp"

namespace Nif
{
    osg::Quat NIFStream::getQuaternion()
    {
        float f[4];
        readLittleEndianBufferOfType<4, float,uint32_t>(inp, (float*)&f);
        osg::Quat quat;
        quat.w() = f[0];
        quat.x() = f[1];
        quat.y() = f[2];
        quat.z() = f[3];
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

    ///Currently specific for 4.0.0.2 and earlier
    bool NIFStream::getBoolean()
    {
        return getInt() != 0;
    }

    ///Read in a string, either from the string table using the index (currently absent) or from the stream using the specified length
    std::string NIFStream::getString()
    {
        return getSizedString();
    }

    // Convenience utility functions: get the versions of the currently read file
    unsigned int NIFStream::getVersion() const { return file->getVersion(); }
    unsigned int NIFStream::getUserVersion() const { return file->getBethVersion(); }
    unsigned int NIFStream::getBethVersion() const { return file->getBethVersion(); }
}
