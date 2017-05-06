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

        void spawnLeveledCreatures(MWWorld::CellStore* cellStore);

        Attack *getLocalAttack(const MWWorld::Ptr& ptr);
        Attack *getDedicatedAttack(const MWWorld::Ptr& ptr);

        void assignAttackTarget(Attack* attack, const MWWorld::Ptr& target);
        void resetAttack(Attack* attack);

        bool getSpellSuccess(std::string spellId, const MWWorld::Ptr& caster);

        void processAttack(Attack attack, const MWWorld::Ptr& attacker);
    };
}

#endif //OPENMW_MECHANICSHELPER_HPP
