#include <components/openmw-mp/Log.hpp>

#include "MechanicsHelper.hpp"
#include "Main.hpp"
using namespace mwmp;

mwmp::MechanicsHelper::MechanicsHelper()
{

}

mwmp::MechanicsHelper::~MechanicsHelper()
{

}

osg::Vec3f MechanicsHelper::getLinearInterpolation(osg::Vec3f start, osg::Vec3f end, float percent)
{
    osg::Vec3f position(percent, percent, percent);

    return (start + osg::componentMultiply(position, (end - start)));
}
