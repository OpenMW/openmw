/*
  Copyright (C) 2020-2021 cc9cii

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
#include "loaddobj.hpp"

#include <cstring>
#include <stdexcept>

#include "reader.hpp"
//#include "writer.hpp"

void ESM4::DefaultObj::load(ESM4::Reader& reader)
{
    mId = reader.getFormIdFromHeader();
    mFlags = reader.hdr().record.flags;

    while (reader.getSubRecordHeader())
    {
        const ESM4::SubRecordHeader& subHdr = reader.subRecordHeader();
        switch (subHdr.typeId)
        {
            case ESM::fourCC("EDID"):
                reader.getZString(mEditorId);
                break; // "DefaultObjectManager"
            case ESM::fourCC("DATA"):
                reader.getFormId(mData.stimpack);
                reader.getFormId(mData.superStimpack);
                reader.getFormId(mData.radX);
                reader.getFormId(mData.radAway);
                reader.getFormId(mData.morphine);
                reader.getFormId(mData.perkParalysis);
                reader.getFormId(mData.playerFaction);
                reader.getFormId(mData.mysteriousStrangerNPC);
                reader.getFormId(mData.mysteriousStrangerFaction);
                reader.getFormId(mData.defaultMusic);
                reader.getFormId(mData.battleMusic);
                reader.getFormId(mData.deathMusic);
                reader.getFormId(mData.successMusic);
                reader.getFormId(mData.levelUpMusic);
                reader.getFormId(mData.playerVoiceMale);
                reader.getFormId(mData.playerVoiceMaleChild);
                reader.getFormId(mData.playerVoiceFemale);
                reader.getFormId(mData.playerVoiceFemaleChild);
                reader.getFormId(mData.eatPackageDefaultFood);
                reader.getFormId(mData.everyActorAbility);
                reader.getFormId(mData.drugWearsOffImageSpace);
                // below FONV only
                if (subHdr.dataSize == 136) // FONV 136/4 = 34 formid
                {
                    reader.getFormId(mData.doctorsBag);
                    reader.getFormId(mData.missFortuneNPC);
                    reader.getFormId(mData.missFortuneFaction);
                    reader.getFormId(mData.meltdownExplosion);
                    reader.getFormId(mData.unarmedForwardPA);
                    reader.getFormId(mData.unarmedBackwardPA);
                    reader.getFormId(mData.unarmedLeftPA);
                    reader.getFormId(mData.unarmedRightPA);
                    reader.getFormId(mData.unarmedCrouchPA);
                    reader.getFormId(mData.unarmedCounterPA);
                    reader.getFormId(mData.spotterEffect);
                    reader.getFormId(mData.itemDetectedEfect);
                    reader.getFormId(mData.cateyeMobileEffectNYI);
                }
                break;
            case ESM::fourCC("DNAM"):
                reader.skipSubRecordData();
                break;
            default:
                throw std::runtime_error("ESM4::DOBJ::load - Unknown subrecord " + ESM::printName(subHdr.typeId));
        }
    }
}

// void ESM4::DefaultObj::save(ESM4::Writer& writer) const
//{
// }

// void ESM4::DefaultObj::blank()
//{
// }
