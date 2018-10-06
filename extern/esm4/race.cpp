/*
  Copyright (C) 2016, 2018 cc9cii

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
#include "race.hpp"

#include <stdexcept>
#include <iostream> // FIXME: debugging only
#include <iomanip>

#include "reader.hpp"
//#include "writer.hpp"

ESM4::Race::Race() : mFormId(0), mFlags(0), mBoundRadius(0.f)
{
    mEditorId.clear();
    mFullName.clear();
    mModel.clear();
    mIcon.clear();

    mData.flags = 0;
}

ESM4::Race::~Race()
{
}

void ESM4::Race::load(ESM4::Reader& reader)
{
    mFormId = reader.hdr().record.id;
    reader.adjustFormId(mFormId);
    mFlags  = reader.hdr().record.flags;

    while (reader.getSubRecordHeader())
    {
        const ESM4::SubRecordHeader& subHdr = reader.subRecordHeader();
        switch (subHdr.typeId)
        {
            case ESM4::SUB_EDID:
            {
                reader.getZString(mEditorId);
                std::cout << "RACE " << mEditorId << std::endl;
                break;
            }
            case ESM4::SUB_FULL:
            {
                if (reader.hasLocalizedStrings())
                    reader.getLocalizedString(mFullName);
                else if (!reader.getZString(mFullName))
                    throw std::runtime_error ("RACE FULL data read error");

                break;
            }
            case ESM4::SUB_MODL: reader.getZString(mModel); break;
            case ESM4::SUB_ICON: reader.getZString(mIcon);  break; // Only in TES4?
          //case ESM4::SUB_DATA: reader.get(mData);         break;
            case ESM4::SUB_MODB: reader.get(mBoundRadius);  break;
          //case ESM4::SUB_MODT:
            case ESM4::SUB_DESC: //skipping...1 <- different lenghts
            {
                if (reader.hasLocalizedStrings())
                    reader.getLocalizedString(mDesc); // TODO check if formid is null
                else if (!reader.getZString(mDesc))
                    throw std::runtime_error ("RACE DESC data read error");

                break;
            }
            case ESM4::SUB_ATTR: //skipping...16 // Only in TES4? guess - 8 attrib each for male/female?
            // Argonian 28 28 1e 32 32 1e 1e 32 28 32 28 28 28 1e 1e 32
            //          40 40 30 50 50 30 30 50 40 50 40 40 40 30 30 50
            // Nord     32 1e 1e 28 28 32 1e 32 32 1e 28 28 28 28 1e 32
            //          50 30 30 40 40 50 30 50 50 30 40 40 40 40 30 50
            //          StrIntWilAglSpdEndPerLuk
            //          Male                    Female
            case ESM4::SUB_CNAM: //skipping...1 // Only in TES4?
                                 // Sheogorath   0x00
                                 // Golden Saint 0x03
                                 // Dark Seducer 0x0C
                                 // Vampire Race 0x00
                                 // Dremora      0x07
                                 // Argonian     0x00
                                 // Nord         0x05
                                 // Breton       0x05
                                 // Wood Elf     0x0D
                                 // khajiit      0x05
                                 // Dark Elf     0x00
                                 // Orc          0x0C
                                 // High Elf     0x0F
                                 // Redguard     0x0D
                                 // Imperial     0x0D
            case ESM4::SUB_DATA: //skipping...36 // ?? different length to TES5
            // Altimer
            //
            // hex 13 05 14 0a 15 05 16 0a 17 05 18 0a ff 00 00 00
            // dec     5    10     5    10     5    10  -1 0
            //     alc   alt   conj  dest  illu  myst  none  unknown (always 00 00)
            //
            // cd cc 8c 3f  : 1.1 height Male
            // cd cc 8c 3f  : 1.1 height Female
            // 00 00 80 3f  : 1.0 weihgt Male
            // 00 00 80 3f  : 1.0 weight Female
            // 01 00 00 00  fist byte 1 means playable? uint32_t flag?
            //
            // Redguard
            //
            // hex 0d 0a 10 0a 12 05 1b 05 0e 0a 1d 05 ff 00 00 00
            // dec    10    10     5     5    10     5 -1
            //     ath   blun  h.arm l.arm blade merch
            //
            //
            // 0a d7 83 3f  : 1.03 height Male
            // 00 00 80 3f  : 1.0  height Female
            // 0a d7 83 3f  : 1.03 weight Male
            // 00 00 80 3f  : 1.0  weight Female
            // 01 00 00 00
            //
            // skill index
            // 0x0C Armorer
            // 0x0D Athletics
            // 0x0E Blade
            // 0x0F Block
            // 0x10 Blunt
            // 0x11 HandToHand
            // 0x12 HeavyArmor
            // 0x13 Alchemy
            // 0x14 Alteration
            // 0x15 Conjuration
            // 0x16 Destruction
            // 0x17 Illusion
            // 0x18 Mysticism
            // 0x19 Restoration
            // 0x1A Acrobatics
            // 0x1B LightArmor
            // 0x1C Marksman
            // 0x1D Mercantile
            // 0x1E Security
            // 0x1F Sneak
            // 0x20 Speechcraft
            case ESM4::SUB_SNAM: //skipping...2 // only in TES4?
            case ESM4::SUB_XNAM: //skipping...8 // only in TES4? Often has similar numbers to VNAM
            case ESM4::SUB_ENAM: //skipping...0 <- different lengthts, maybe formids for EYES?
            case ESM4::SUB_HNAM: //skipping...0 <- different lengthts, maybe formids for HAIR?
            case ESM4::SUB_VNAM: //skipping...8 // equipment type flags meant to be uint32 ???
                                                // GLOB reference? shows up in SCRO in sript
                                                // records and CTDA in INFO records
            {
                std::cout << "RACE " << ESM4::printName(subHdr.typeId) << " skipping..." << subHdr.dataSize << std::endl;
    // For debugging only
//#if 0
                unsigned char mDataBuf[256/*bufSize*/];
                reader.get(&mDataBuf[0], subHdr.dataSize);

                std::ostringstream ss;
                for (unsigned int i = 0; i < subHdr.dataSize; ++i)
                {
                    //if (mDataBuf[i] > 64 && mDataBuf[i] < 91)
                        //ss << (char)(mDataBuf[i]) << " ";
                    //else
                        ss << std::setfill('0') << std::setw(2) << std::hex << (int)(mDataBuf[i]);
                    if ((i & 0x000f) == 0xf)
                        ss << "\n";
                    else if (i < 256/*bufSize*/-1)
                        ss << " ";
                }
                std::cout << ss.str() << std::endl;

                //reader.skipSubRecordData();
                break;
            }
//#endif
            case ESM4::SUB_DNAM: //skipping...8 // decapitate armor, 2 formids
            case ESM4::SUB_FGGA: //skipping...120 // prob face gen stuff
            case ESM4::SUB_FGGS: //skipping...200 // prob face gen stuff
            case ESM4::SUB_FGTS: //skipping...200 // prob face gen stuff
            case ESM4::SUB_FNAM: //skipping...0 // start marker female model
            case ESM4::SUB_INDX: //skipping...4 // marker preceding egt models? uint32 always 0
            case ESM4::SUB_MNAM: //skipping...0 // start marker male model
            case ESM4::SUB_NAM0: //skipping...0 // start marker head data
            case ESM4::SUB_NAM1: //skipping...0 // strat marker egt models
            case ESM4::SUB_PNAM: //skipping...4 // face gen main clamp float
            case ESM4::SUB_SPLO: //skipping...4 // bonus spell formid (TES5 may have SPCT and multiple SPLO)
            case ESM4::SUB_UNAM: //skipping...4 // face gen face clamp float
            case ESM4::SUB_YNAM: // FO3
            case ESM4::SUB_NAM2: // FO3
            case ESM4::SUB_VTCK: // FO3
            case ESM4::SUB_MODT: // FO3
            case ESM4::SUB_MODD: // FO3
            case ESM4::SUB_ONAM: // FO3
            {

                //std::cout << "RACE " << ESM4::printName(subHdr.typeId) << " skipping..." << subHdr.dataSize << std::endl;
                reader.skipSubRecordData();
                break;
            }
            default:
                throw std::runtime_error("ESM4::RACE::load - Unknown subrecord " + ESM4::printName(subHdr.typeId));
        }
    }
}

//void ESM4::Race::save(ESM4::Writer& writer) const
//{
//}

//void ESM4::Race::blank()
//{
//}
