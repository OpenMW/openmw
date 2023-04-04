#include "loadskil.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

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
    const std::string Skill::sSkillNameIds[Length] = {
        "sSkillBlock",
        "sSkillArmorer",
        "sSkillMediumarmor",
        "sSkillHeavyarmor",
        "sSkillBluntweapon",
        "sSkillLongblade",
        "sSkillAxe",
        "sSkillSpear",
        "sSkillAthletics",
        "sSkillEnchant",
        "sSkillDestruction",
        "sSkillAlteration",
        "sSkillIllusion",
        "sSkillConjuration",
        "sSkillMysticism",
        "sSkillRestoration",
        "sSkillAlchemy",
        "sSkillUnarmored",
        "sSkillSecurity",
        "sSkillSneak",
        "sSkillAcrobatics",
        "sSkillLightarmor",
        "sSkillShortblade",
        "sSkillMarksman",
        "sSkillMercantile",
        "sSkillSpeechcraft",
        "sSkillHandtohand",
    };
    const std::string Skill::sIconNames[Length] = {
        "combat_block.dds",
        "combat_armor.dds",
        "combat_mediumarmor.dds",
        "combat_heavyarmor.dds",
        "combat_blunt.dds",
        "combat_longblade.dds",
        "combat_axe.dds",
        "combat_spear.dds",
        "combat_athletics.dds",
        "magic_enchant.dds",
        "magic_destruction.dds",
        "magic_alteration.dds",
        "magic_illusion.dds",
        "magic_conjuration.dds",
        "magic_mysticism.dds",
        "magic_restoration.dds",
        "magic_alchemy.dds",
        "magic_unarmored.dds",
        "stealth_security.dds",
        "stealth_sneak.dds",
        "stealth_acrobatics.dds",
        "stealth_lightarmor.dds",
        "stealth_shortblade.dds",
        "stealth_marksman.dds",
        "stealth_mercantile.dds",
        "stealth_speechcraft.dds",
        "stealth_handtohand.dds",
    };
    const std::array<Skill::SkillEnum, Skill::Length> Skill::sSkillIds
        = { { Block, Armorer, MediumArmor, HeavyArmor, BluntWeapon, LongBlade, Axe, Spear, Athletics, Enchant,
            Destruction, Alteration, Illusion, Conjuration, Mysticism, Restoration, Alchemy, Unarmored, Security, Sneak,
            Acrobatics, LightArmor, ShortBlade, Marksman, Mercantile, Speechcraft, HandToHand } };

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
        if (index == -1)
            return RefId();
        return RefId::index(sRecordId, static_cast<std::uint32_t>(index));
    }
}
