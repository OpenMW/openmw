#ifndef OPENMW_MECHANICSHELPER_HPP
#define OPENMW_MECHANICSHELPER_HPP

#include <components/openmw-mp/Base/BaseStructs.hpp>

#include <osg/Vec3f>

namespace mwmp
{
    class MechanicsHelper
    {
    public:

        MechanicsHelper();
        ~MechanicsHelper();

        osg::Vec3f getLinearInterpolation(osg::Vec3f start, osg::Vec3f end, float percent);

        void processAttack(const MWWorld::Ptr& attacker, Attack attack);
    };
}

#endif //OPENMW_MECHANICSHELPER_HPP
