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

  Also see https://tes5edit.github.io/fopdoc/ for FO3/FONV specific details.

*/
#ifndef ESM4_DOBJ_H
#define ESM4_DOBJ_H

#include <cstdint>
#include <string>

#include <components/esm/defs.hpp>
#include <components/esm/formid.hpp>

namespace ESM4
{
    class Reader;
    class Writer;

    struct Defaults
    {
        ESM::FormId stimpack;
        ESM::FormId superStimpack;
        ESM::FormId radX;
        ESM::FormId radAway;
        ESM::FormId morphine;
        ESM::FormId perkParalysis;
        ESM::FormId playerFaction;
        ESM::FormId mysteriousStrangerNPC;
        ESM::FormId mysteriousStrangerFaction;
        ESM::FormId defaultMusic;
        ESM::FormId battleMusic;
        ESM::FormId deathMusic;
        ESM::FormId successMusic;
        ESM::FormId levelUpMusic;
        ESM::FormId playerVoiceMale;
        ESM::FormId playerVoiceMaleChild;
        ESM::FormId playerVoiceFemale;
        ESM::FormId playerVoiceFemaleChild;
        ESM::FormId eatPackageDefaultFood;
        ESM::FormId everyActorAbility;
        ESM::FormId drugWearsOffImageSpace;
        // below FONV only
        ESM::FormId doctorsBag;
        ESM::FormId missFortuneNPC;
        ESM::FormId missFortuneFaction;
        ESM::FormId meltdownExplosion;
        ESM::FormId unarmedForwardPA;
        ESM::FormId unarmedBackwardPA;
        ESM::FormId unarmedLeftPA;
        ESM::FormId unarmedRightPA;
        ESM::FormId unarmedCrouchPA;
        ESM::FormId unarmedCounterPA;
        ESM::FormId spotterEffect;
        ESM::FormId itemDetectedEfect;
        ESM::FormId cateyeMobileEffectNYI;
    };

    struct DefaultObj
    {
        ESM::FormId mId; // from the header
        std::uint32_t mFlags; // from the header, see enum type RecordFlag for details

        std::string mEditorId;

        Defaults mData;

        void load(ESM4::Reader& reader);
        // void save(ESM4::Writer& writer) const;

        // void blank();
        static constexpr ESM::RecNameInts sRecordId = ESM::RecNameInts::REC_DOBJ4;
    };
}

#endif // ESM4_DOBJ_H
