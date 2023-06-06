#include "loadskil.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

#include <components/misc/strings/algorithm.hpp>

namespace ESM
{
    const RefId Skill::Block = Skill::indexToRefId(0);
    const RefId Skill::Armorer = Skill::indexToRefId(1);
    const RefId Skill::MediumArmor = Skill::indexToRefId(2);
    const RefId Skill::HeavyArmor = Skill::indexToRefId(3);
    const RefId Skill::BluntWeapon = Skill::indexToRefId(4);
    const RefId Skill::LongBlade = Skill::indexToRefId(5);
    const RefId Skill::Axe = Skill::indexToRefId(6);
    const RefId Skill::Spear = Skill::indexToRefId(7);
    const RefId Skill::Athletics = Skill::indexToRefId(8);
    const RefId Skill::Enchant = Skill::indexToRefId(9);
    const RefId Skill::Destruction = Skill::indexToRefId(10);
    const RefId Skill::Alteration = Skill::indexToRefId(11);
    const RefId Skill::Illusion = Skill::indexToRefId(12);
    const RefId Skill::Conjuration = Skill::indexToRefId(13);
    const RefId Skill::Mysticism = Skill::indexToRefId(14);
    const RefId Skill::Restoration = Skill::indexToRefId(15);
    const RefId Skill::Alchemy = Skill::indexToRefId(16);
    const RefId Skill::Unarmored = Skill::indexToRefId(17);
    const RefId Skill::Security = Skill::indexToRefId(18);
    const RefId Skill::Sneak = Skill::indexToRefId(19);
    const RefId Skill::Acrobatics = Skill::indexToRefId(20);
    const RefId Skill::LightArmor = Skill::indexToRefId(21);
    const RefId Skill::ShortBlade = Skill::indexToRefId(22);
    const RefId Skill::Marksman = Skill::indexToRefId(23);
    const RefId Skill::Mercantile = Skill::indexToRefId(24);
    const RefId Skill::Speechcraft = Skill::indexToRefId(25);
    const RefId Skill::HandToHand = Skill::indexToRefId(26);

    const std::string Skill::sSkillNames[Length] = {
        "Block",
        "Armorer",
        "Mediumarmor",
        "Heavyarmor",
        "Bluntweapon",
        "Longblade",
        "Axe",
        "Spear",
        "Athletics",
        "Enchant",
        "Destruction",
        "Alteration",
        "Illusion",
        "Conjuration",
        "Mysticism",
        "Restoration",
        "Alchemy",
        "Unarmored",
        "Security",
        "Sneak",
        "Acrobatics",
        "Lightarmor",
        "Shortblade",
        "Marksman",
        "Mercantile",
        "Speechcraft",
        "Handtohand",
    };

    int Skill::stringToSkillId(std::string_view skill)
    {
        for (int id = 0; id < Skill::Length; ++id)
            if (Misc::StringUtils::ciEqual(sSkillNames[id], skill))
                return id;

        throw std::logic_error("No such skill: " + std::string(skill));
    }

    void Skill::load(ESMReader& esm, bool& isDeleted)
    {
        isDeleted = false; // Skill record can't be deleted now (may be changed in the future)
        mRecordFlags = esm.getRecordFlags();

        bool hasIndex = false;
        bool hasData = false;
        while (esm.hasMoreSubs())
        {
            esm.getSubName();
            switch (esm.retSubName().toInt())
            {
                case fourCC("INDX"):
                    esm.getHT(mIndex);
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
        else if (mIndex < 0 || mIndex >= Length)
            esm.fail("Invalid INDX");
        if (!hasData)
            esm.fail("Missing SKDT");

        // create an ID from the index and the name (only used in the editor and likely to change in the
        // future)
        mId = indexToRefId(mIndex);
    }

    void Skill::save(ESMWriter& esm, bool /*isDeleted*/) const
    {
        esm.writeHNT("INDX", mIndex);
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

    RefId Skill::indexToRefId(int index)
    {
        if (index < 0 || index >= Length)
            return RefId();
        return RefId::index(sRecordId, static_cast<std::uint32_t>(index));
    }
}
