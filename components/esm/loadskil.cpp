#include "loadskil.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace ESM
{
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
    const boost::array<Skill::SkillEnum, Skill::Length> Skill::sSkillIds = {{
        Block,
        Armorer,
        MediumArmor,
        HeavyArmor,
        BluntWeapon,
        LongBlade,
        Axe,
        Spear,
        Athletics,
        Enchant,
        Destruction,
        Alteration,
        Illusion,
        Conjuration,
        Mysticism,
        Restoration,
        Alchemy,
        Unarmored,
        Security,
        Sneak,
        Acrobatics,
        LightArmor,
        ShortBlade,
        Marksman,
        Mercantile,
        Speechcraft,
        HandToHand
    }};

void Skill::load(ESMReader &esm)
{
    esm.getHNT(mIndex, "INDX");
    esm.getHNT(mData, "SKDT", 24);
    mDescription = esm.getHNOString("DESC");
}
void Skill::save(ESMWriter &esm)
{
    esm.writeHNT("INDX", mIndex);
    esm.writeHNT("SKDT", mData, 24);
    esm.writeHNOString("DESC", mDescription);
}
}
