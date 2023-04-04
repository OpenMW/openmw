/*
  Copyright (C) 2015-2020 cc9cii

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  cc9cii cc9c@iinet.net.au

  Much of the information on the data structures are based on the information
  from Tes4Mod:Mod_File_Format and Tes5Mod:File_Formats but also refined by
  trial & error.  See http://en.uesp.net/wiki for details.

*/
#ifndef OPENMW_COMPONENTS_ESM4_MAGICEFFECTID_H
#define OPENMW_COMPONENTS_ESM4_MAGICEFFECTID_H

#include "components/esm/fourcc.hpp"

namespace ESM4
{
    using ESM::fourCC;

    enum MagicEffectID
    {
        // Alteration
        EFI_BRDN = fourCC("BRDN"),
        EFI_FTHR = fourCC("FTHR"),
        EFI_FISH = fourCC("FISH"),
        EFI_FRSH = fourCC("FRSH"),
        EFI_OPEN = fourCC("OPNN"),
        EFI_SHLD = fourCC("SHLD"),
        EFI_LISH = fourCC("LISH"),
        EFI_WABR = fourCC("WABR"),
        EFI_WAWA = fourCC("WAWA"),

        // Conjuration
        EFI_BABO = fourCC("BABO"), // Bound Boots
        EFI_BACU = fourCC("BACU"), // Bound Cuirass
        EFI_BAGA = fourCC("BAGA"), // Bound Gauntlets
        EFI_BAGR = fourCC("BAGR"), // Bound Greaves
        EFI_BAHE = fourCC("BAHE"), // Bound Helmet
        EFI_BASH = fourCC("BASH"), // Bound Shield
        EFI_BWAX = fourCC("BWAX"), // Bound Axe
        EFI_BWBO = fourCC("BWBO"), // Bound Bow
        EFI_BWDA = fourCC("BWDA"), // Bound Dagger
        EFI_BWMA = fourCC("BWMA"), // Bound Mace
        EFI_BWSW = fourCC("BWSW"), // Bound Sword
        EFI_Z001 = fourCC("Z001"), // Summon Rufio's Ghost
        EFI_Z002 = fourCC("Z002"), // Summon Ancestor Guardian
        EFI_Z003 = fourCC("Z003"), // Summon Spiderling
        EFI_Z005 = fourCC("Z005"), // Summon Bear
        EFI_ZCLA = fourCC("ZCLA"), // Summon Clannfear
        EFI_ZDAE = fourCC("ZDAE"), // Summon Daedroth
        EFI_ZDRE = fourCC("ZDRE"), // Summon Dremora
        EFI_ZDRL = fourCC("ZDRL"), // Summon Dremora Lord
        EFI_ZFIA = fourCC("ZFIA"), // Summon Flame Atronach
        EFI_ZFRA = fourCC("ZFRA"), // Summon Frost Atronach
        EFI_ZGHO = fourCC("ZGHO"), // Summon Ghost
        EFI_ZHDZ = fourCC("ZHDZ"), // Summon Headless Zombie
        EFI_ZLIC = fourCC("ZLIC"), // Summon Lich
        EFI_ZSCA = fourCC("ZSCA"), // Summon Scamp
        EFI_ZSKE = fourCC("ZSKE"), // Summon Skeleton
        EFI_ZSKA = fourCC("ZSKA"), // Summon Skeleton Guardian
        EFI_ZSKH = fourCC("ZSKH"), // Summon Skeleton Hero
        EFI_ZSKC = fourCC("ZSKC"), // Summon Skeleton Champion
        EFI_ZSPD = fourCC("ZSPD"), // Summon Spider Daedra
        EFI_ZSTA = fourCC("ZSTA"), // Summon Storm Atronach
        EFI_ZWRA = fourCC("ZWRA"), // Summon Faded Wraith
        EFI_ZWRL = fourCC("ZWRL"), // Summon Gloom Wraith
        EFI_ZXIV = fourCC("ZXIV"), // Summon Xivilai
        EFI_ZZOM = fourCC("ZZOM"), // Summon Zombie
        EFI_TURN = fourCC("TURN"), // Turn Undead

        // Destruction
        EFI_DGAT = fourCC("DGAT"), // Damage Attribute
        EFI_DGFA = fourCC("DGFA"), // Damage Fatigue
        EFI_DGHE = fourCC("DGHE"), // Damage Health
        EFI_DGSP = fourCC("DGSP"), // Damage Magicka
        EFI_DIAR = fourCC("DIAR"), // Disintegrate Armor
        EFI_DIWE = fourCC("DIWE"), // Disintegrate Weapon
        EFI_DRAT = fourCC("DRAT"), // Drain Attribute
        EFI_DRFA = fourCC("DRFA"), // Drain Fatigue
        EFI_DRHE = fourCC("DRHE"), // Drain Health
        EFI_DRSP = fourCC("DRSP"), // Drain Magicka
        EFI_DRSK = fourCC("DRSK"), // Drain Skill
        EFI_FIDG = fourCC("FIDG"), // Fire Damage
        EFI_FRDG = fourCC("FRDG"), // Frost Damage
        EFI_SHDG = fourCC("SHDG"), // Shock Damage
        EFI_WKDI = fourCC("WKDI"), // Weakness to Disease
        EFI_WKFI = fourCC("WKFI"), // Weakness to Fire
        EFI_WKFR = fourCC("WKFR"), // Weakness to Frost
        EFI_WKMA = fourCC("WKMA"), // Weakness to Magic
        EFI_WKNW = fourCC("WKNW"), // Weakness to Normal Weapons
        EFI_WKPO = fourCC("WKPO"), // Weakness to Poison
        EFI_WKSH = fourCC("WKSH"), // Weakness to Shock

        // Illusion
        EFI_CALM = fourCC("CALM"), // Calm
        EFI_CHML = fourCC("CHML"), // Chameleon
        EFI_CHRM = fourCC("CHRM"), // Charm
        EFI_COCR = fourCC("COCR"), // Command Creature
        EFI_COHU = fourCC("COHU"), // Command Humanoid
        EFI_DEMO = fourCC("DEMO"), // Demoralize
        EFI_FRNZ = fourCC("FRNZ"), // Frenzy
        EFI_INVI = fourCC("INVI"), // Invisibility
        EFI_LGHT = fourCC("LGHT"), // Light
        EFI_NEYE = fourCC("NEYE"), // Night-Eye
        EFI_PARA = fourCC("PARA"), // Paralyze
        EFI_RALY = fourCC("RALY"), // Rally
        EFI_SLNC = fourCC("SLNC"), // Silence

        // Mysticism
        EFI_DTCT = fourCC("DTCT"), // Detect Life
        EFI_DSPL = fourCC("DSPL"), // Dispel
        EFI_REDG = fourCC("REDG"), // Reflect Damage
        EFI_RFLC = fourCC("RFLC"), // Reflect Spell
        EFI_STRP = fourCC("STRP"), // Soul Trap
        EFI_SABS = fourCC("SABS"), // Spell Absorption
        EFI_TELE = fourCC("TELE"), // Telekinesis

        // Restoration
        EFI_ABAT = fourCC("ABAT"), // Absorb Attribute
        EFI_ABFA = fourCC("ABFA"), // Absorb Fatigue
        EFI_ABHe = fourCC("ABHe"), // Absorb Health
        EFI_ABSP = fourCC("ABSP"), // Absorb Magicka
        EFI_ABSK = fourCC("ABSK"), // Absorb Skill
        EFI_1400 = fourCC("1400"), // Cure Disease
        EFI_CUPA = fourCC("CUPA"), // Cure Paralysis
        EFI_CUPO = fourCC("CUPO"), // Cure Poison
        EFI_FOAT = fourCC("FOAT"), // Fortify Attribute
        EFI_FOFA = fourCC("FOFA"), // Fortify Fatigue
        EFI_FOHE = fourCC("FOHE"), // Fortify Health
        EFI_FOSP = fourCC("FOSP"), // Fortify Magicka
        EFI_FOSK = fourCC("FOSK"), // Fortify Skill
        EFI_RSDI = fourCC("RSDI"), // Resist Disease
        EFI_RSFI = fourCC("RSFI"), // Resist Fire
        EFI_RSFR = fourCC("RSFR"), // Resist Frost
        EFI_RSMA = fourCC("RSMA"), // Resist Magic
        EFI_RSNW = fourCC("RSNW"), // Resist Normal Weapons
        EFI_RSPA = fourCC("RSPA"), // Resist Paralysis
        EFI_RSPO = fourCC("RSPO"), // Resist Poison
        EFI_RSSH = fourCC("RSSH"), // Resist Shock
        EFI_REAT = fourCC("REAT"), // Restore Attribute
        EFI_REFA = fourCC("REFA"), // Restore Fatigue
        EFI_REHE = fourCC("REHE"), // Restore Health
        EFI_RESP = fourCC("RESP"), // Restore Magicka

        // Effects
        EFI_LOCK = fourCC("LOCK"), // Lock Lock
        EFI_SEFF = fourCC("SEFF"), // Script Effect
        EFI_Z020 = fourCC("Z020"), // Summon 20 Extra
        EFI_MYHL = fourCC("MYHL"), // Summon Mythic Dawn Helmet
        EFI_MYTH = fourCC("MYTH"), // Summon Mythic Dawn Armor
        EFI_REAN = fourCC("REAN"), // Reanimate
        EFI_DISE = fourCC("DISE"), // Disease Info
        EFI_POSN = fourCC("POSN"), // Poison Info
        EFI_DUMY = fourCC("DUMY"), // Mehrunes Dagon Custom Effect
        EFI_STMA = fourCC("STMA"), // Stunted Magicka
        EFI_SUDG = fourCC("SUDG"), // Sun Damage
        EFI_VAMP = fourCC("VAMP"), // Vampirism
        EFI_DARK = fourCC("DARK"), // Darkness
        EFI_RSWD = fourCC("RSWD") // Resist Water Damage
    };
}

#endif // OPENMW_COMPONENTS_ESM4_MAGICEFFECTID_H
