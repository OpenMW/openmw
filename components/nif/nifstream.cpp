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
}
