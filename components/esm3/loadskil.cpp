#include "loadskil.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

#include <components/misc/strings/algorithm.hpp>

namespace ESM
{
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
