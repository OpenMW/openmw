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
#include "soun.hpp"

#include <stdexcept>

#include "reader.hpp"
//#include "writer.hpp"

ESM4::Sound::Sound() : mFormId(0), mFlags(0)
{
    mEditorId.clear();
    mSoundFile.clear();
}

ESM4::Sound::~Sound()
{
}

void ESM4::Sound::load(ESM4::Reader& reader)
{
    mFormId = reader.hdr().record.id;
    reader.adjustFormId(mFormId);
    mFlags  = reader.hdr().record.flags;

    while (reader.getSubRecordHeader())
    {
        const ESM4::SubRecordHeader& subHdr = reader.subRecordHeader();
        switch (subHdr.typeId)
        {
            case ESM4::SUB_EDID: reader.getZString(mEditorId);  break;
            case ESM4::SUB_FNAM: reader.getZString(mSoundFile); break;
            case ESM4::SUB_SNDX: reader.get(mData); break;
            case ESM4::SUB_SNDD:
            case ESM4::SUB_OBND: // TES5 only
            case ESM4::SUB_SDSC: // TES5 only
            case ESM4::SUB_ANAM: // FO3
            case ESM4::SUB_GNAM: // FO3
            case ESM4::SUB_HNAM: // FO3
            case ESM4::SUB_RNAM: // FONV
            {
                //std::cout << "SOUN " << ESM4::printName(subHdr.typeId) << " skipping..." << std::endl;
                reader.skipSubRecordData();
                break;
            }
            default:
                throw std::runtime_error("ESM4::SOUN::load - Unknown subrecord " + ESM4::printName(subHdr.typeId));
        }
    }
}

//void ESM4::Sound::save(ESM4::Writer& writer) const
//{
//}

//void ESM4::Sound::blank()
//{
//}
