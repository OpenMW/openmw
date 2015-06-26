#include "autocalc.hpp"

#include <cmath>

#include <components/esm/attr.hpp>
#include <components/esm/loadmgef.hpp>
#include <components/esm/loadskil.hpp>
#include <components/esm/loadrace.hpp>
#include <components/esm/loadclas.hpp>
#include <components/esm/loadnpc.hpp>

#include "autocalcspell.hpp"

namespace
{
    int is_even(double d)
    {
        double int_part;

        modf(d / 2.0, &int_part);
        return 2.0 * int_part == d;
    }

    int round_ieee_754(double d)
    {
        double i = floor(d);
        d -= i;

        if(d < 0.5)
            return static_cast<int>(i);
        if(d > 0.5)
            return static_cast<int>(i) + 1;
        if(is_even(i))
            return static_cast<int>(i);
        return static_cast<int>(i) + 1;
    }
}

namespace AutoCalc
{
    void autoCalcAttributesImpl (const ESM::NPC* npc,
            const ESM::Race *race, const ESM::Class *class_, int level, StatsBase& stats, StoreCommon *store)
    {
        // race bonus
        bool male = (npc->mFlags & ESM::NPC::Female) == 0;

        for (int i=0; i<ESM::Attribute::Length; ++i)
        {
            const ESM::Race::MaleFemale& attribute = race->mData.mAttributeValues[i];
            stats.setAttribute(i, male ? attribute.mMale : attribute.mFemale);
        }

        // class bonus
        for (int i=0; i<2; ++i)
        {
            int attribute = class_->mData.mAttribute[i];
            if (attribute>=0 && attribute<ESM::Attribute::Length)
                stats.setAttribute(attribute, stats.getBaseAttribute(attribute) + 10);
            // else log an error?
        }

        // skill bonus
        for (int attribute=0; attribute < ESM::Attribute::Length; ++attribute)
        {
            float modifierSum = 0;

            for (int j=0; j<ESM::Skill::Length; ++j)
            {
                const ESM::Skill* skill = store->findSkill(j);

                if (skill->mData.mAttribute != attribute)
                    continue;

                // is this a minor or major skill?
                float add=0.2f;
                for (int k=0; k<5; ++k)
                {
                    if (class_->mData.mSkills[k][0] == j)
                        add=0.5;
                }
                for (int k=0; k<5; ++k)
                {
                    if (class_->mData.mSkills[k][1] == j)
                        add=1.0;
                }
                modifierSum += add;
            }
            stats.setAttribute(attribute,
                    std::min(round_ieee_754(stats.getBaseAttribute(attribute) + (level-1) * modifierSum), 100) );
        }
    }

    /**
     * @brief autoCalculateSkills
     *
     * Skills are calculated with following formulae ( http://www.uesp.net/wiki/Morrowind:NPCs#Skills ):
     *
     * Skills: (Level - 1) × (Majority Multiplier + Specialization Multiplier)
     *
     *         The Majority Multiplier is 1.0 for a Major or Minor Skill, or 0.1 for a Miscellaneous Skill.
     *
     *         The Specialization Multiplier is 0.5 for a Skill in the same Specialization as the class,
     *         zero for other Skills.
     *
     * and by adding class, race, specialization bonus.
     */
    void autoCalcSkillsImpl (const ESM::NPC* npc,
            const ESM::Race *race, const ESM::Class *class_, int level, StatsBase& stats, StoreCommon *store)
    {
        for (int i = 0; i < 2; ++i)
        {
            int bonus = (i==0) ? 10 : 25;

            for (int i2 = 0; i2 < 5; ++i2)
            {
                int index = class_->mData.mSkills[i2][i];
                if (index >= 0 && index < ESM::Skill::Length)
                {
                    stats.setBaseSkill (index, bonus);
                }
            }
        }

        for (int skillIndex = 0; skillIndex < ESM::Skill::Length; ++skillIndex)
        {
            float majorMultiplier = 0.1f;
            float specMultiplier = 0.0f;

            int raceBonus = 0;
            int specBonus = 0;

            for (int raceSkillIndex = 0; raceSkillIndex < 7; ++raceSkillIndex)
            {
                if (race->mData.mBonus[raceSkillIndex].mSkill == skillIndex)
                {
                    raceBonus = race->mData.mBonus[raceSkillIndex].mBonus;
                    break;
                }
            }

            for (int k = 0; k < 5; ++k)
            {
                // is this a minor or major skill?
                if ((class_->mData.mSkills[k][0] == skillIndex) || (class_->mData.mSkills[k][1] == skillIndex))
                {
                    majorMultiplier = 1.0f;
                    break;
                }
            }

            // is this skill in the same Specialization as the class?
            const ESM::Skill* skill = store->findSkill(skillIndex);
            if (skill->mData.mSpecialization == class_->mData.mSpecialization)
            {
                specMultiplier = 0.5f;
                specBonus = 5;
            }

            stats.setBaseSkill(skillIndex,
                  std::min(
                    round_ieee_754(
                            stats.getBaseSkill(skillIndex)
                    + 5
                    + raceBonus
                    + specBonus
                    +(int(level)-1) * (majorMultiplier + specMultiplier)), 100)); // Must gracefully handle level 0
        }
    }

    unsigned short autoCalculateHealth(int level, const ESM::Class *class_, const StatsBase& stats)
    {
        // initial health
        int strength = stats.getBaseAttribute(ESM::Attribute::Strength);
        int endurance = stats.getBaseAttribute(ESM::Attribute::Endurance);

        int multiplier = 3;

        if (class_->mData.mSpecialization == ESM::Class::Combat)
            multiplier += 2;
        else if (class_->mData.mSpecialization == ESM::Class::Stealth)
            multiplier += 1;

        if (class_->mData.mAttribute[0] == ESM::Attribute::Endurance
            || class_->mData.mAttribute[1] == ESM::Attribute::Endurance)
            multiplier += 1;

        return static_cast<unsigned short>(floor(0.5f * (strength + endurance)) + multiplier * (level-1));
    }

    void autoCalculateSpells(const ESM::Race *race, StatsBase& stats, StoreCommon *store)
    {
        int skills[ESM::Skill::Length];
        for (int i=0; i<ESM::Skill::Length; ++i)
            skills[i] = stats.getBaseSkill(i);

        int attributes[ESM::Attribute::Length];
        for (int i=0; i<ESM::Attribute::Length; ++i)
            attributes[i] = stats.getBaseAttribute(i);

        std::vector<std::string> spells = autoCalcNpcSpells(skills, attributes, race, store);
        for (std::vector<std::string>::iterator it = spells.begin(); it != spells.end(); ++it)
            stats.addSpell(*it);
    }

    StatsBase::StatsBase() {}

    StatsBase::~StatsBase() {}
}
