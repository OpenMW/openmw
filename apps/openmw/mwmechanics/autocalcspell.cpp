#include "autocalcspell.hpp"

#include <limits>

#include "../mwworld/esmstore.hpp"

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"

#include "spellutil.hpp"

namespace MWMechanics
{

    struct SchoolCaps
    {
        int mCount;
        int mLimit;
        bool mReachedLimit;
        int mMinCost;
        std::string mWeakestSpell;
    };

    std::vector<std::string> autoCalcNpcSpells(const int *actorSkills, const int *actorAttributes, const ESM::Race* race)
    {
        const MWWorld::Store<ESM::GameSetting>& gmst = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();
        static const float fNPCbaseMagickaMult = gmst.find("fNPCbaseMagickaMult")->mValue.getFloat();
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
                iAutoSpellSchoolMax[i] = gmst.find(gmstName)->mValue.getInteger();
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
            caps.mMinCost = std::numeric_limits<int>::max();
            caps.mWeakestSpell.clear();
            schoolCaps[i] = caps;
        }

        std::vector<std::string> selectedSpells;

        const MWWorld::Store<ESM::Spell> &spells =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>();

        // Note: the algorithm heavily depends on the traversal order of the spells. For vanilla-compatible results the
        // Store must preserve the record ordering as it was in the content files.
        for (const ESM::Spell& spell : spells)
        {
            if (spell.mData.mType != ESM::Spell::ST_Spell)
                continue;
            if (!(spell.mData.mFlags & ESM::Spell::F_Autocalc))
                continue;
            static const int iAutoSpellTimesCanCast = gmst.find("iAutoSpellTimesCanCast")->mValue.getInteger();
            int spellCost = MWMechanics::calcSpellCost(spell);
            if (baseMagicka < iAutoSpellTimesCanCast * spellCost)
                continue;

            if (race && race->mPowers.exists(spell.mId))
                continue;

            if (!attrSkillCheck(&spell, actorSkills, actorAttributes))
                continue;

            int school;
            float skillTerm;
            calcWeakestSchool(&spell, actorSkills, school, skillTerm);
            assert(school >= 0 && school < 6);
            SchoolCaps& cap = schoolCaps[school];

            if (cap.mReachedLimit && spellCost <= cap.mMinCost)
                continue;

            static const float fAutoSpellChance = gmst.find("fAutoSpellChance")->mValue.getFloat();
            if (calcAutoCastChance(&spell, actorSkills, actorAttributes, school) < fAutoSpellChance)
                continue;

            selectedSpells.push_back(spell.mId);

            if (cap.mReachedLimit)
            {
                std::vector<std::string>::iterator found = std::find(selectedSpells.begin(), selectedSpells.end(), cap.mWeakestSpell);
                if (found != selectedSpells.end())
                    selectedSpells.erase(found);

                cap.mMinCost = std::numeric_limits<int>::max();
                for (const std::string& testSpellName : selectedSpells)
                {
                    const ESM::Spell* testSpell = spells.find(testSpellName);
                    int testSpellCost = MWMechanics::calcSpellCost(*testSpell);

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
                            testSpellCost < cap.mMinCost)
                    {
                        cap.mMinCost = testSpellCost;
                        cap.mWeakestSpell = testSpell->mId;
                    }
                }
            }
            else
            {
                cap.mCount += 1;
                if (cap.mCount == cap.mLimit)
                    cap.mReachedLimit = true;

                if (spellCost < cap.mMinCost)
                {
                    cap.mWeakestSpell = spell.mId;
                    cap.mMinCost = spellCost;
                }
            }
        }

        return selectedSpells;
    }

    std::vector<std::string> autoCalcPlayerSpells(const int* actorSkills, const int* actorAttributes, const ESM::Race* race)
    {
        const MWWorld::ESMStore& esmStore = MWBase::Environment::get().getWorld()->getStore();

        static const float fPCbaseMagickaMult = esmStore.get<ESM::GameSetting>().find("fPCbaseMagickaMult")->mValue.getFloat();

        float baseMagicka = fPCbaseMagickaMult * actorAttributes[ESM::Attribute::Intelligence];
        bool reachedLimit = false;
        const ESM::Spell* weakestSpell = nullptr;
        int minCost = std::numeric_limits<int>::max();

        std::vector<std::string> selectedSpells;

        const MWWorld::Store<ESM::Spell> &spells = esmStore.get<ESM::Spell>();
        for (const ESM::Spell& spell : spells)
        {
            if (spell.mData.mType != ESM::Spell::ST_Spell)
                continue;
            if (!(spell.mData.mFlags & ESM::Spell::F_PCStart))
                continue;

            int spellCost = MWMechanics::calcSpellCost(spell);
            if (reachedLimit && spellCost <= minCost)
                continue;
            if (race && std::find(race->mPowers.mList.begin(), race->mPowers.mList.end(), spell.mId) != race->mPowers.mList.end())
                continue;
            if (baseMagicka < spellCost)
                continue;

            static const float fAutoPCSpellChance = esmStore.get<ESM::GameSetting>().find("fAutoPCSpellChance")->mValue.getFloat();
            if (calcAutoCastChance(&spell, actorSkills, actorAttributes, -1) < fAutoPCSpellChance)
                continue;

            if (!attrSkillCheck(&spell, actorSkills, actorAttributes))
                continue;

            selectedSpells.push_back(spell.mId);

            if (reachedLimit)
            {
                std::vector<std::string>::iterator it = std::find(selectedSpells.begin(), selectedSpells.end(), weakestSpell->mId);
                if (it != selectedSpells.end())
                    selectedSpells.erase(it);

                minCost = std::numeric_limits<int>::max();
                for (const std::string& testSpellName : selectedSpells)
                {
                    const ESM::Spell* testSpell = esmStore.get<ESM::Spell>().find(testSpellName);
                    int testSpellCost = MWMechanics::calcSpellCost(*testSpell);
                    if (testSpellCost < minCost)
                    {
                        minCost = testSpellCost;
                        weakestSpell = testSpell;
                    }
                }
            }
            else
            {
                if (spellCost < minCost)
                {
                    weakestSpell = &spell;
                    minCost = MWMechanics::calcSpellCost(*weakestSpell);
                }
                static const unsigned int iAutoPCSpellMax = esmStore.get<ESM::GameSetting>().find("iAutoPCSpellMax")->mValue.getInteger();
                if (selectedSpells.size() == iAutoPCSpellMax)
                    reachedLimit = true;
            }
        }

        return selectedSpells;
    }

    bool attrSkillCheck (const ESM::Spell* spell, const int* actorSkills, const int* actorAttributes)
    {
        for (const auto& spellEffect : spell->mEffects.mList)
        {
            const ESM::MagicEffect* magicEffect = MWBase::Environment::get().getWorld()->getStore().get<ESM::MagicEffect>().find(spellEffect.mEffectID);
            static const int iAutoSpellAttSkillMin = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("iAutoSpellAttSkillMin")->mValue.getInteger();

            if ((magicEffect->mData.mFlags & ESM::MagicEffect::TargetSkill))
            {
                assert (spellEffect.mSkill >= 0 && spellEffect.mSkill < ESM::Skill::Length);
                if (actorSkills[spellEffect.mSkill] < iAutoSpellAttSkillMin)
                    return false;
            }

            if ((magicEffect->mData.mFlags & ESM::MagicEffect::TargetAttribute))
            {
                assert (spellEffect.mAttribute >= 0 && spellEffect.mAttribute < ESM::Attribute::Length);
                if (actorAttributes[spellEffect.mAttribute] < iAutoSpellAttSkillMin)
                    return false;
            }
        }

        return true;
    }

    void calcWeakestSchool (const ESM::Spell* spell, const int* actorSkills, int& effectiveSchool, float& skillTerm)
    {
        // Morrowind for some reason uses a formula slightly different from magicka cost calculation
        float minChance = std::numeric_limits<float>::max();
        for (const ESM::ENAMstruct& effect : spell->mEffects.mList)
        {
            const ESM::MagicEffect* magicEffect = MWBase::Environment::get().getWorld()->getStore().get<ESM::MagicEffect>().find(effect.mEffectID);

            int minMagn = 1;
            int maxMagn = 1;
            if (!(magicEffect->mData.mFlags & ESM::MagicEffect::NoMagnitude))
            {
                minMagn = effect.mMagnMin;
                maxMagn = effect.mMagnMax;
            }

            int duration = 0;
            if (!(magicEffect->mData.mFlags & ESM::MagicEffect::NoDuration))
                duration = effect.mDuration;
            if (!(magicEffect->mData.mFlags & ESM::MagicEffect::AppliedOnce))
                duration = std::max(1, duration);

            static const float fEffectCostMult = MWBase::Environment::get().getWorld()->getStore()
                .get<ESM::GameSetting>().find("fEffectCostMult")->mValue.getFloat();

            float x = 0.5 * (std::max(1, minMagn) + std::max(1, maxMagn));
            x *= 0.1 * magicEffect->mData.mBaseCost;
            x *= 1 + duration;
            x += 0.05 * std::max(1, effect.mArea) * magicEffect->mData.mBaseCost;
            x *= fEffectCostMult;

            if (effect.mRange == ESM::RT_Target)
                x *= 1.5f;

            float s = 2.f * actorSkills[spellSchoolToSkill(magicEffect->mData.mSchool)];
            if (s - x < minChance)
            {
                minChance = s - x;
                effectiveSchool = magicEffect->mData.mSchool;
                skillTerm = s;
            }
        }
    }

    float calcAutoCastChance(const ESM::Spell *spell, const int *actorSkills, const int *actorAttributes, int effectiveSchool)
    {
        if (spell->mData.mType != ESM::Spell::ST_Spell)
            return 100.f;

        if (spell->mData.mFlags & ESM::Spell::F_Always)
            return 100.f;

        float skillTerm = 0;
        if (effectiveSchool != -1)
            skillTerm = 2.f * actorSkills[spellSchoolToSkill(effectiveSchool)];
        else
            calcWeakestSchool(spell, actorSkills, effectiveSchool, skillTerm); // Note effectiveSchool is unused after this

        float castChance = skillTerm - MWMechanics::calcSpellCost(*spell)
            + 0.2f * actorAttributes[ESM::Attribute::Willpower]
            + 0.1f * actorAttributes[ESM::Attribute::Luck];
        return castChance;
    }
}
