/*
  Copyright (C) 2019 cc9cii

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
#include "bsgn.hpp"

#include <stdexcept>
#include <iostream> // FIXME: testing only
#include "formid.hpp" // FIXME: testing only

#include "common.hpp"
#include "reader.hpp"
#include "writer.hpp"

ESM4::BirthSign::BirthSign() : mFormId(0), mFlags(0)//, mData(0), mAdditionalPart(0)
{
    mEditorId.clear();
    mFullName.clear();
    //mModel.clear();
}

ESM4::BirthSign::~BirthSign()
{
}

void ESM4::BirthSign::load(ESM4::Reader& reader)
{
    mFormId = reader.hdr().record.id;
    reader.adjustFormId(mFormId);
    mFlags  = reader.hdr().record.flags;



    while (reader.getSubRecordHeader())
    {
        const ESM4::SubRecordHeader& subHdr = reader.subRecordHeader();
        switch (subHdr.typeId)
        {
            case ESM4::SUB_EDID: reader.getZString(mEditorId); break;
      /*  case ESM::SREC_NAME:
            mId = esm.getHString();
            hasName = true;
            break;*/
        case ESM4::SUB_FULL:
            reader.getZString(mFullName);
            std::cout<< mFullName<<std::endl;
            break;
        case ESM4::SUB_SPLO:
           { FormId id;
            reader.getFormId(id);
            std::cout<< formIdToString(id)<<std::endl;
           // mSpell.push_back(id);
        }
            break;
        case ESM4::SUB_ICON:
        {  std::string iconname;
reader.getZString(iconname);
            std::cout<< iconname<<std::endl;
            }
            break;
        case ESM4::SUB_FNAM:
            reader.getZString(mName);
            break;
        case ESM4::SUB_TNAM:
            reader.getZString(mTexture);
            break;
        case ESM4::SUB_DESC:
            reader.getZString(mDescription);
            break;
        case ESM4::SUB_NPCS:
        {
            std::string power;
            reader.getZString(power);
            mPowers.push_back(power);
            break;
        }
            /*
            case ESM4::SUB_FULL:
            {
                if (reader.hasLocalizedStrings())
                    reader.getLocalizedString(mFullName);
                else if (!reader.getZString(mFullName))
                    throw std::runtime_error ("HDPT FULL data read error");

                break;
            }
            case ESM4::SUB_DATA: reader.get(mData); break;
            case ESM4::SUB_MODL: reader.getZString(mModel); break;
            case ESM4::SUB_HNAM: reader.get(mAdditionalPart); break;
            case ESM4::SUB_PNAM:
            case ESM4::SUB_MODS:
            case ESM4::SUB_MODT:
            case ESM4::SUB_NAM0:
            case ESM4::SUB_NAM1:
            case ESM4::SUB_RNAM:
            case ESM4::SUB_TNAM:
            {
                //std::cout << "HDPT " << ESM4::printName(subHdr.typeId) << " skipping..." << std::endl;
                reader.skipSubRecordData();
                break;
            }*/
            default:
                throw std::runtime_error("ESM4::BirthSign::load - Unknown subrecord " + ESM4::printName(subHdr.typeId));
        }
    }
}

void ESM4::BirthSign::save(ESM4::Writer& writer) const
{
}

//void ESM4::HeadPart::blank()
//{
//}
