#include "nifstream.hpp"
//For error reporting
#include "niffile.hpp"

namespace Nif
{
    osg::Quat NIFStream::getQuaternion()
    {
        float f[4];
        readLittleEndianBufferOfType<4, float>(inp, f);
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

    ///Booleans in 4.0.0.2 (Morrowind format) and earlier are 4 byte, while in 4.1.0.0+ they're 1 byte.
    bool NIFStream::getBoolean()
    {
        return getVersion() < generateVersion(4,1,0,0) ? getInt() != 0 : getChar() != 0;
    }

    ///Read in a string, either from the string table using the index or from the stream using the specified length
    std::string NIFStream::getString()
    {
        return getVersion() < generateVersion(20,1,0,1) ? getSizedString() : file->getString(getUInt());
    }


    // Convenience utility functions: get the versions of the currently read file
    unsigned int NIFStream::getVersion() const { return file->getVersion(); }
    unsigned int NIFStream::getUserVersion() const { return file->getBethVersion(); }
    unsigned int NIFStream::getBethVersion() const { return file->getBethVersion(); }
}
