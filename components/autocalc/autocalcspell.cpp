#include "autocalcspell.hpp"

#include <climits>
#include <cfloat>
#include <set>

#include <components/esm/attr.hpp>
#include <components/esm/loadmgef.hpp>
#include <components/esm/loadrace.hpp>
#include <components/esm/loadclas.hpp>
#include <components/esm/loadspel.hpp>
#include <components/esm/loadnpc.hpp>

#include "autocalc.hpp"

namespace AutoCalc
{

    struct SchoolCaps
    {
        int mCount;
        int mLimit;
        bool mReachedLimit;
        int mMinCost;
        std::string mWeakestSpell;
    };

    std::vector<std::string> autoCalcNpcSpells(const int *actorSkills,
            const int *actorAttributes, const ESM::Race* race, StoreCommon *store)
    {
        static const float fNPCbaseMagickaMult = store->findGmstFloat("fNPCbaseMagickaMult");
        float baseMagicka = fNPCbaseMagickaMult * actorAttributes[ESM::Attribute::Intelligence];

        static const std::string schools[] = {
            "alteration", "conjuration", "destruction", "illusion", "mysticism", "restoration"
        };
        static int iAutoSpellSchoolMax[6];
        static bool init = false;
        if (!init)
        {
            for (int i=0; i<6; ++i)
            {
                const std::string& gmstName = "iAutoSpell" + schools[i] + "Max";
                iAutoSpellSchoolMax[i] = store->findGmstInt(gmstName);
            }
            init = true;
        }

        std::map<int, SchoolCaps> schoolCaps;
        for (int i=0; i<6; ++i)
        {
            SchoolCaps caps;
            caps.mCount = 0;
            caps.mLimit = iAutoSpellSchoolMax[i];
            caps.mReachedLimit = iAutoSpellSchoolMax[i] <= 0;
            caps.mMinCost = INT_MAX;
            caps.mWeakestSpell.clear();
            schoolCaps[i] = caps;
        }

        std::vector<std::string> selectedSpells;
        std::vector<ESM::Spell*> spells;
        store->getSpells(spells);

        // Note: the algorithm heavily depends on the traversal order of the spells. For vanilla-compatible results the
        // Store must preserve the record ordering as it was in the content files.
        for (std::vector<ESM::Spell*>::const_iterator iter = spells.begin(); iter != spells.end(); ++iter)
        {
            ESM::Spell* spell = *iter;

            if (spell->mData.mType != ESM::Spell::ST_Spell)
                continue;
            if (!(spell->mData.mFlags & ESM::Spell::F_Autocalc))
                continue;
            static const int iAutoSpellTimesCanCast = store->findGmstInt("iAutoSpellTimesCanCast");
            if (baseMagicka < iAutoSpellTimesCanCast * spell->mData.mCost)
                continue;

            if (race && race->mPowers.exists(spell->mId))
                continue;

            if (!attrSkillCheck(spell, actorSkills, actorAttributes, store))
                continue;

            int school;
            float skillTerm;
            calcWeakestSchool(spell, actorSkills, school, skillTerm, store);
            assert(school >= 0 && school < 6);
            SchoolCaps& cap = schoolCaps[school];

            if (cap.mReachedLimit && spell->mData.mCost <= cap.mMinCost)
                continue;

            static const float fAutoSpellChance = store->findGmstFloat("fAutoSpellChance");
            if (calcAutoCastChance(spell, actorSkills, actorAttributes, school, store) < fAutoSpellChance)
                continue;

            selectedSpells.push_back(spell->mId);

            if (cap.mReachedLimit)
            {
                std::vector<std::string>::iterator found = std::find(selectedSpells.begin(), selectedSpells.end(), cap.mWeakestSpell);
                if (found != selectedSpells.end())
                    selectedSpells.erase(found);

                cap.mMinCost = INT_MAX;
                for (std::vector<std::string>::iterator weakIt = selectedSpells.begin(); weakIt != selectedSpells.end(); ++weakIt)
                {
                    std::vector<ESM::Spell*>::const_iterator it = spells.begin();
                    for (; it != spells.end(); ++it)
                    {
                        if ((*it)->mId == *weakIt)
                            break;
                    }

                    if (it == spells.end())
                        continue;

                    const ESM::Spell* testSpell = *it;

                    //int testSchool;
                    //float dummySkillTerm;
                    //calcWeakestSchool(testSpell, actorSkills, testSchool, dummySkillTerm);

                    // Note: if there are multiple spells with the same cost, we pick the first one we found.
                    // So the algorithm depends on the iteration order of the outer loop.
                    if (
                            // There is a huge bug here. It is not checked that weakestSpell is of the correct school.
                            // As result multiple SchoolCaps could have the same mWeakestSpell. Erasing the weakest spell would then fail if another school
                            // already erased it, and so the number of spells would often exceed the sum of limits.
                            // This bug cannot be fixed without significantly changing the results of the spell autocalc, which will not have been playtested.
                            //testSchool == school &&
                            testSpell->mData.mCost < cap.mMinCost)
                    {
                        cap.mMinCost = testSpell->mData.mCost;
                        cap.mWeakestSpell = testSpell->mId;
                    }
                }
            }
            else
            {
                cap.mCount += 1;
                if (cap.mCount == cap.mLimit)
                    cap.mReachedLimit = true;

                if (spell->mData.mCost < cap.mMinCost)
                {
                    cap.mWeakestSpell = spell->mId;
                    cap.mMinCost = spell->mData.mCost;
                }
            }
        }

        return selectedSpells;
    }

    bool attrSkillCheck (const ESM::Spell* spell,
            const int* actorSkills, const int* actorAttributes, StoreCommon *store)
    {
        const std::vector<ESM::ENAMstruct>& effects = spell->mEffects.mList;
        for (std::vector<ESM::ENAMstruct>::const_iterator effectIt = effects.begin(); effectIt != effects.end(); ++effectIt)
        {
            const ESM::MagicEffect* magicEffect = store->findMagicEffect(effectIt->mEffectID);
            static const int iAutoSpellAttSkillMin = store->findGmstInt("iAutoSpellAttSkillMin");

            if ((magicEffect->mData.mFlags & ESM::MagicEffect::TargetSkill))
            {
                assert (effectIt->mSkill >= 0 && effectIt->mSkill < ESM::Skill::Length);
                if (actorSkills[effectIt->mSkill] < iAutoSpellAttSkillMin)
                    return false;
            }

            if ((magicEffect->mData.mFlags & ESM::MagicEffect::TargetAttribute))
            {
                assert (effectIt->mAttribute >= 0 && effectIt->mAttribute < ESM::Attribute::Length);
                if (actorAttributes[effectIt->mAttribute] < iAutoSpellAttSkillMin)
                    return false;
            }
        }

        return true;
    }

    ESM::Skill::SkillEnum mapSchoolToSkill(int school)
    {
        std::map<int, ESM::Skill::SkillEnum> schoolSkillMap; // maps spell school to skill id
        schoolSkillMap[0] = ESM::Skill::Alteration;
        schoolSkillMap[1] = ESM::Skill::Conjuration;
        schoolSkillMap[3] = ESM::Skill::Illusion;
        schoolSkillMap[2] = ESM::Skill::Destruction;
        schoolSkillMap[4] = ESM::Skill::Mysticism;
        schoolSkillMap[5] = ESM::Skill::Restoration;
        assert(schoolSkillMap.find(school) != schoolSkillMap.end());
        return schoolSkillMap[school];
    }

    void calcWeakestSchool (const ESM::Spell* spell,
            const int* actorSkills, int& effectiveSchool, float& skillTerm, StoreCommon *store)
    {
        float minChance = FLT_MAX;

        const ESM::EffectList& effects = spell->mEffects;
        for (std::vector<ESM::ENAMstruct>::const_iterator it = effects.mList.begin(); it != effects.mList.end(); ++it)
        {
            const ESM::ENAMstruct& effect = *it;
            float x = static_cast<float>(effect.mDuration);

            const ESM::MagicEffect* magicEffect = store->findMagicEffect(effect.mEffectID);
            if (!(magicEffect->mData.mFlags & ESM::MagicEffect::UncappedDamage))
                x = std::max(1.f, x);

            x *= 0.1f * magicEffect->mData.mBaseCost;
            x *= 0.5f * (effect.mMagnMin + effect.mMagnMax);
            x += effect.mArea * 0.05f * magicEffect->mData.mBaseCost;
            if (effect.mRange == ESM::RT_Target)
                x *= 1.5f;

            static const float fEffectCostMult = store->findGmstFloat("fEffectCostMult");
            x *= fEffectCostMult;

            float s = 2.f * actorSkills[mapSchoolToSkill(magicEffect->mData.mSchool)];
            if (s - x < minChance)
            {
                minChance = s - x;
                effectiveSchool = magicEffect->mData.mSchool;
                skillTerm = s;
            }
        }
    }

    float calcAutoCastChance(const ESM::Spell *spell,
            const int *actorSkills, const int *actorAttributes, int effectiveSchool, StoreCommon *store)
    {
        if (spell->mData.mType != ESM::Spell::ST_Spell)
            return 100.f;

        if (spell->mData.mFlags & ESM::Spell::F_Always)
            return 100.f;

        float skillTerm = 0;
        if (effectiveSchool != -1)
            skillTerm = 2.f * actorSkills[mapSchoolToSkill(effectiveSchool)];
        else
            calcWeakestSchool(spell, actorSkills, effectiveSchool, skillTerm, store); // Note effectiveSchool is unused after this

        float castChance = skillTerm - spell->mData.mCost + 0.2f * actorAttributes[ESM::Attribute::Willpower] + 0.1f * actorAttributes[ESM::Attribute::Luck];
        return castChance;
    }
}
