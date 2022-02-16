/*
  Copyright (C) 2020 cc9cii

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
#ifndef ESM4_ACTOR_H
#define ESM4_ACTOR_H

#include <cstdint>

#include "formid.hpp"

namespace ESM4
{
#pragma pack(push, 1)
    struct AIData        // NPC_, CREA
    {
        std::uint8_t  aggression;
        std::uint8_t  confidence;
        std::uint8_t  energyLevel;
        std::uint8_t  responsibility;
        std::uint32_t aiFlags;
        std::uint8_t  trainSkill;
        std::uint8_t  trainLevel;
        std::uint16_t unknown;
    };

    struct AttributeValues
    {
        std::uint8_t  strength;
        std::uint8_t  intelligence;
        std::uint8_t  willpower;
        std::uint8_t  agility;
        std::uint8_t  speed;
        std::uint8_t  endurance;
        std::uint8_t  personality;
        std::uint8_t  luck;
    };

    struct ACBS_TES4
    {
        std::uint32_t flags;
        std::uint16_t baseSpell;
        std::uint16_t fatigue;
        std::uint16_t barterGold;
        std::int16_t  levelOrOffset;
        std::uint16_t calcMin;
        std::uint16_t calcMax;
        std::uint32_t padding1;
        std::uint32_t padding2;
    };

    struct ACBS_FO3
    {
        std::uint32_t flags;
        std::uint16_t fatigue;
        std::uint16_t barterGold;
        std::int16_t  levelOrMult;
        std::uint16_t calcMinlevel;
        std::uint16_t calcMaxlevel;
        std::uint16_t speedMultiplier;
        float         karma;
        std::int16_t  dispositionBase;
        std::uint16_t templateFlags;
    };

    struct ACBS_TES5
    {
        std::uint32_t flags;
        std::uint16_t magickaOffset;
        std::uint16_t staminaOffset;
        std::uint16_t levelOrMult;     // TODO: check if int16_t
        std::uint16_t calcMinlevel;
        std::uint16_t calcMaxlevel;
        std::uint16_t speedMultiplier;
        std::uint16_t dispositionBase; // TODO: check if int16_t
        std::uint16_t templateFlags;
        std::uint16_t healthOffset;
        std::uint16_t bleedoutOverride;
    };

    union ActorBaseConfig
    {
        ACBS_TES4 tes4;
        ACBS_FO3  fo3;
        ACBS_TES5 tes5;
    };

    struct ActorFaction
    {
        FormId       faction;
        std::int8_t  rank;
        std::uint8_t unknown1;
        std::uint8_t unknown2;
        std::uint8_t unknown3;
    };
#pragma pack(pop)

    struct BodyTemplate // TES5
    {
        // 0x00000001 - Head
        // 0x00000002 - Hair
        // 0x00000004 - Body
        // 0x00000008 - Hands
        // 0x00000010 - Forearms
        // 0x00000020 - Amulet
        // 0x00000040 - Ring
        // 0x00000080 - Feet
        // 0x00000100 - Calves
        // 0x00000200 - Shield
        // 0x00000400 - Tail
        // 0x00000800 - Long Hair
        // 0x00001000 - Circlet
        // 0x00002000 - Ears
        // 0x00004000 - Body AddOn 3
        // 0x00008000 - Body AddOn 4
        // 0x00010000 - Body AddOn 5
        // 0x00020000 - Body AddOn 6
        // 0x00040000 - Body AddOn 7
        // 0x00080000 - Body AddOn 8
        // 0x00100000 - Decapitate Head
        // 0x00200000 - Decapitate
        // 0x00400000 - Body AddOn 9
        // 0x00800000 - Body AddOn 10
        // 0x01000000 - Body AddOn 11
        // 0x02000000 - Body AddOn 12
        // 0x04000000 - Body AddOn 13
        // 0x08000000 - Body AddOn 14
        // 0x10000000 - Body AddOn 15
        // 0x20000000 - Body AddOn 16
        // 0x40000000 - Body AddOn 17
        // 0x80000000 - FX01
        std::uint32_t bodyPart;
        std::uint8_t flags;
        std::uint8_t unknown1; // probably padding
        std::uint8_t unknown2; // probably padding
        std::uint8_t unknown3; // probably padding
        std::uint32_t type; // 0 = light, 1 = heavy, 2 = none (cloth?)
    };
}

#endif // ESM4_ACTOR_H
