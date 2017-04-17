#ifndef OPENMW_MECHANICSHELPER_HPP
#define OPENMW_MECHANICSHELPER_HPP

#include <osg/Vec3f>

namespace mwmp
{
    class MechanicsHelper
    {
    public:

        MechanicsHelper();
        ~MechanicsHelper();

        osg::Vec3f getLinearInterpolation(osg::Vec3f start, osg::Vec3f end, float percent);
    };
}

#endif //OPENMW_MECHANICSHELPER_HPP
