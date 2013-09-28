
#include "npcstats.hpp"

#include <cmath>
#include <stdexcept>

#include <components/esm/loadskil.hpp>
#include <components/esm/loadclas.hpp>
#include <components/esm/loadgmst.hpp>

#include <components/esm_store/store.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

MWMechanics::NpcStats::NpcStats()
: mMovementFlags (0), mDrawState (DrawState_Nothing)
{}

MWMechanics::DrawState_ MWMechanics::NpcStats::getDrawState() const
{
    return mDrawState;
}

void MWMechanics::NpcStats::setDrawState (DrawState_ state)
{
    mDrawState = state;
}

bool MWMechanics::NpcStats::getMovementFlag (Flag flag) const
{
    return mMovementFlags & flag;
}

void MWMechanics::NpcStats::setMovementFlag (Flag flag, bool state)
{
    if (state)
        mMovementFlags |= flag;
    else
        mMovementFlags &= ~flag;
}

const MWMechanics::Stat<float>& MWMechanics::NpcStats::getSkill (int index) const
{
    if (index<0 || index>=27)
        throw std::runtime_error ("skill index out of range");

    return mSkill[index];
}

MWMechanics::Stat<float>& MWMechanics::NpcStats::getSkill (int index)
{
    if (index<0 || index>=27)
        throw std::runtime_error ("skill index out of range");

    return mSkill[index];
}

std::map<std::string, int>& MWMechanics::NpcStats::getFactionRanks()
{
    return mFactionRank;
}

const std::map<std::string, int>& MWMechanics::NpcStats::getFactionRanks() const
{
    return mFactionRank;
}

float MWMechanics::NpcStats::getSkillGain (int skillIndex, const ESM::Class& class_, int usageType,
    int level) const
{
    if (level<0)
        level = static_cast<int> (getSkill (skillIndex).getBase());

    const ESM::Skill *skill = MWBase::Environment::get().getWorld()->getStore().skills.find (skillIndex);

    float skillFactor = 1;

    if (usageType>=4)
        throw std::runtime_error ("skill usage type out of range");

    if (usageType>0)
    {
        skillFactor = skill->data.useValue[usageType];

        if (skillFactor<=0)
            throw std::runtime_error ("invalid skill gain factor");
    }

    float typeFactor =
        MWBase::Environment::get().getWorld()->getStore().gameSettings.find ("fMiscSkillBonus")->f;

    for (int i=0; i<5; ++i)
        if (class_.data.skills[i][0]==skillIndex)
        {
            typeFactor =
                MWBase::Environment::get().getWorld()->getStore().gameSettings.find ("fMinorSkillBonus")->f;

            break;
        }

    for (int i=0; i<5; ++i)
        if (class_.data.skills[i][1]==skillIndex)
        {
            typeFactor =
                MWBase::Environment::get().getWorld()->getStore().gameSettings.find ("fMajorSkillBonus")->f;

            break;
        }

    if (typeFactor<=0)
        throw std::runtime_error ("invalid skill type factor");

    float specialisationFactor = 1;

    if (skill->data.specialization==class_.data.specialization)
    {
        specialisationFactor =
            MWBase::Environment::get().getWorld()->getStore().gameSettings.find ("fSpecialSkillBonus")->f;

        if (specialisationFactor<=0)
            throw std::runtime_error ("invalid skill specialisation factor");
    }

    return 1.0 / (level +1) * (1.0 / skillFactor) * typeFactor * specialisationFactor;
}

void MWMechanics::NpcStats::useSkill (int skillIndex, const ESM::Class& class_, int usageType)
{
    float base = getSkill (skillIndex).getBase();

    int level = static_cast<int> (base);

    base += getSkillGain (skillIndex, class_, usageType);

    if (static_cast<int> (base)!=level)
        base = level+1;

    getSkill (skillIndex).setBase (base);
}
