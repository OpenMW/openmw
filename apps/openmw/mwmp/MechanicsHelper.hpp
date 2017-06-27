#ifndef OPENMW_MECHANICSHELPER_HPP
#define OPENMW_MECHANICSHELPER_HPP

#include <components/openmw-mp/Base/BaseStructs.hpp>

#include <osg/Vec3f>


namespace MechanicsHelper
{
    osg::Vec3f getLinearInterpolation(osg::Vec3f start, osg::Vec3f end, float percent);

    void spawnLeveledCreatures(MWWorld::CellStore* cellStore);

    mwmp::Attack *getLocalAttack(const MWWorld::Ptr& ptr);
    mwmp::Attack *getDedicatedAttack(const MWWorld::Ptr& ptr);

    MWWorld::Ptr getPlayerPtr(const mwmp::Target& target);

    void assignAttackTarget(mwmp::Attack* attack, const MWWorld::Ptr& target);
    void resetAttack(mwmp::Attack* attack);

    bool getSpellSuccess(std::string spellId, const MWWorld::Ptr& caster);

    void processAttack(mwmp::Attack attack, const MWWorld::Ptr& attacker);
}


#endif //OPENMW_MECHANICSHELPER_HPP
