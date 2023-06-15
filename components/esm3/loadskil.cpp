#include "loadskil.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

#include <components/misc/strings/algorithm.hpp>

namespace ESM
{
    const RefId Skill::Block = RefId::stringRefId("Block");
    const RefId Skill::Armorer = RefId::stringRefId("Armorer");
    const RefId Skill::MediumArmor = RefId::stringRefId("MediumArmor");
    const RefId Skill::HeavyArmor = RefId::stringRefId("HeavyArmor");
    const RefId Skill::BluntWeapon = RefId::stringRefId("BluntWeapon");
    const RefId Skill::LongBlade = RefId::stringRefId("LongBlade");
    const RefId Skill::Axe = RefId::stringRefId("Axe");
    const RefId Skill::Spear = RefId::stringRefId("Spear");
    const RefId Skill::Athletics = RefId::stringRefId("Athletics");
    const RefId Skill::Enchant = RefId::stringRefId("Enchant");
    const RefId Skill::Destruction = RefId::stringRefId("Destruction");
    const RefId Skill::Alteration = RefId::stringRefId("Alteration");
    const RefId Skill::Illusion = RefId::stringRefId("Illusion");
    const RefId Skill::Conjuration = RefId::stringRefId("Conjuration");
    const RefId Skill::Mysticism = RefId::stringRefId("Mysticism");
    const RefId Skill::Restoration = RefId::stringRefId("Restoration");
    const RefId Skill::Alchemy = RefId::stringRefId("Alchemy");
    const RefId Skill::Unarmored = RefId::stringRefId("Unarmored");
    const RefId Skill::Security = RefId::stringRefId("Security");
    const RefId Skill::Sneak = RefId::stringRefId("Sneak");
    const RefId Skill::Acrobatics = RefId::stringRefId("Acrobatics");
    const RefId Skill::LightArmor = RefId::stringRefId("LightArmor");
    const RefId Skill::ShortBlade = RefId::stringRefId("ShortBlade");
    const RefId Skill::Marksman = RefId::stringRefId("Marksman");
    const RefId Skill::Mercantile = RefId::stringRefId("Mercantile");
    const RefId Skill::Speechcraft = RefId::stringRefId("Speechcraft");
    const RefId Skill::HandToHand = RefId::stringRefId("HandToHand");

    void Skill::load(ESMReader& esm, bool& isDeleted)
    {
        isDeleted = false; // Skill record can't be deleted now (may be changed in the future)
        mRecordFlags = esm.getRecordFlags();

        bool hasIndex = false;
        bool hasData = false;
        int index = -1;
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
                    esm.getHTSized<24>(mData);
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

        // create an ID from the index and the name (only used in the editor and likely to change in the
        // future)
        mId = indexToRefId(index);
    }

    void Skill::save(ESMWriter& esm, bool /*isDeleted*/) const
    {
        esm.writeHNT("INDX", refIdToIndex(mId));
        esm.writeHNT("SKDT", mData, 24);
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

    static const RefId sSkills[] = {
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
