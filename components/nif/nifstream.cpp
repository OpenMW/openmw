#include "nifstream.hpp"

#include "niffile.hpp"

namespace Nif
{
    osg::Quat NIFStream::getQuaternion()
    {
        float f[4];
        readBufferOfType<4, float>(mStream, f);
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

    bool NIFStream::getBoolean()
    {
        return getVersion() < generateVersion(4, 1, 0, 0) ? getInt() != 0 : getChar() != 0;
    }

    std::string NIFStream::getString()
    {
        return getVersion() < generateVersion(20, 1, 0, 1) ? getSizedString() : mReader.getString(getUInt());
    }

    unsigned int NIFStream::getVersion() const
    {
        return mReader.getVersion();
    }
    unsigned int NIFStream::getUserVersion() const
    {
        return mReader.getBethVersion();
    }
    unsigned int NIFStream::getBethVersion() const
    {
        return mReader.getBethVersion();
    }
}
