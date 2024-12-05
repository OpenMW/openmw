#include "loadskil.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

#include <components/misc/concepts.hpp>
#include <components/misc/strings/algorithm.hpp>

#include <cstdint>

namespace ESM
{
    const SkillId Skill::Block("Block");
    const SkillId Skill::Armorer("Armorer");
    const SkillId Skill::MediumArmor("MediumArmor");
    const SkillId Skill::HeavyArmor("HeavyArmor");
    const SkillId Skill::BluntWeapon("BluntWeapon");
    const SkillId Skill::LongBlade("LongBlade");
    const SkillId Skill::Axe("Axe");
    const SkillId Skill::Spear("Spear");
    const SkillId Skill::Athletics("Athletics");
    const SkillId Skill::Enchant("Enchant");
    const SkillId Skill::Destruction("Destruction");
    const SkillId Skill::Alteration("Alteration");
    const SkillId Skill::Illusion("Illusion");
    const SkillId Skill::Conjuration("Conjuration");
    const SkillId Skill::Mysticism("Mysticism");
    const SkillId Skill::Restoration("Restoration");
    const SkillId Skill::Alchemy("Alchemy");
    const SkillId Skill::Unarmored("Unarmored");
    const SkillId Skill::Security("Security");
    const SkillId Skill::Sneak("Sneak");
    const SkillId Skill::Acrobatics("Acrobatics");
    const SkillId Skill::LightArmor("LightArmor");
    const SkillId Skill::ShortBlade("ShortBlade");
    const SkillId Skill::Marksman("Marksman");
    const SkillId Skill::Mercantile("Mercantile");
    const SkillId Skill::Speechcraft("Speechcraft");
    const SkillId Skill::HandToHand("HandToHand");

    template <Misc::SameAsWithoutCvref<Skill::SKDTstruct> T>
    void decompose(T&& v, const auto& f)
    {
        f(v.mAttribute, v.mSpecialization, v.mUseValue);
    }

    void Skill::load(ESMReader& esm, bool& isDeleted)
    {
        isDeleted = false; // Skill record can't be deleted now (may be changed in the future)
        mRecordFlags = esm.getRecordFlags();

        bool hasIndex = false;
        bool hasData = false;
        int32_t index = -1;
        while (esm.hasMoreSubs())
        {
            esm.getSubName();
            switch (esm.retSubName().toInt())
            {
                case fourCC("INDX"):
                    esm.getHT(index);
                    hasIndex = true;
                    break;
                case fourCC("SKDT"):
                    esm.getSubComposite(mData);
                    hasData = true;
                    break;
                case fourCC("DESC"):
                    mDescription = esm.getHString();
                    break;
                default:
                    esm.fail("Unknown subrecord");
            }
        }
        if (!hasIndex)
            esm.fail("Missing INDX");
        else if (index < 0 || index >= Length)
            esm.fail("Invalid INDX");
        if (!hasData)
            esm.fail("Missing SKDT");

        mId = *indexToRefId(index).getIf<SkillId>();
    }

    void Skill::save(ESMWriter& esm, bool /*isDeleted*/) const
    {
        esm.writeHNT("INDX", refIdToIndex(mId));
        esm.writeNamedComposite("SKDT", mData);
        esm.writeHNOString("DESC", mDescription);
    }

    void Skill::blank()
    {
        mRecordFlags = 0;
        mData.mAttribute = 0;
        mData.mSpecialization = 0;
        mData.mUseValue[0] = mData.mUseValue[1] = mData.mUseValue[2] = mData.mUseValue[3] = 1.0;
        mDescription.clear();
    }

    static const RefId sSkills[Skill::Length] = {
        Skill::Block,
        Skill::Armorer,
        Skill::MediumArmor,
        Skill::HeavyArmor,
        Skill::BluntWeapon,
        Skill::LongBlade,
        Skill::Axe,
        Skill::Spear,
        Skill::Athletics,
        Skill::Enchant,
        Skill::Destruction,
        Skill::Alteration,
        Skill::Illusion,
        Skill::Conjuration,
        Skill::Mysticism,
        Skill::Restoration,
        Skill::Alchemy,
        Skill::Unarmored,
        Skill::Security,
        Skill::Sneak,
        Skill::Acrobatics,
        Skill::LightArmor,
        Skill::ShortBlade,
        Skill::Marksman,
        Skill::Mercantile,
        Skill::Speechcraft,
        Skill::HandToHand,
    };

    RefId Skill::indexToRefId(int index)
    {
        if (index < 0 || index >= Length)
            return RefId();
        return sSkills[index];
    }

    int Skill::refIdToIndex(RefId id)
    {
        for (int i = 0; i < Length; ++i)
        {
            if (sSkills[i] == id)
                return i;
        }
        return -1;
    }

    const std::array<RefId, MagicSchool::Length> sMagicSchools = {
        Skill::Alteration,
        Skill::Conjuration,
        Skill::Destruction,
        Skill::Illusion,
        Skill::Mysticism,
        Skill::Restoration,
    };

    RefId MagicSchool::indexToSkillRefId(int index)
    {
        if (index < 0 || index >= Length)
            return {};
        return sMagicSchools[index];
    }

    int MagicSchool::skillRefIdToIndex(RefId id)
    {
        for (size_t i = 0; i < sMagicSchools.size(); ++i)
        {
            if (id == sMagicSchools[i])
                return static_cast<int>(i);
        }
        return -1;
    }
}
