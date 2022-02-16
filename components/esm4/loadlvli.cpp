/*
  Copyright (C) 2016, 2018-2020 cc9cii

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
#include "loadlvli.hpp"

#include <stdexcept>
#include <iostream> // FIXME: for debugging

#include "reader.hpp"
//#include "writer.hpp"

ESM4::LevelledItem::LevelledItem() : mFormId(0), mFlags(0), mChanceNone(0), mHasLvlItemFlags(false),
    mLvlItemFlags(0), mData(0)
{
    mEditorId.clear();
}

ESM4::LevelledItem::~LevelledItem()
{
}

void ESM4::LevelledItem::load(ESM4::Reader& reader)
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
            case ESM4::SUB_LVLD: reader.get(mChanceNone); break;
            case ESM4::SUB_LVLF: reader.get(mLvlItemFlags); mHasLvlItemFlags = true; break;
            case ESM4::SUB_DATA: reader.get(mData); break;
            case ESM4::SUB_LVLO:
            {
                static LVLO lvlo;
                if (subHdr.dataSize != 12)
                {
                    if (subHdr.dataSize == 8)
                    {
                        reader.get(lvlo.level);
                        reader.get(lvlo.item);
                        reader.get(lvlo.count);
//                        std::cout << "LVLI " << mEditorId << " LVLO lev " << lvlo.level << ", item " << lvlo.item
//                                  << ", count " << lvlo.count << std::endl;
                        break;
                    }
                    else
                        throw std::runtime_error("ESM4::LVLI::load - " + mEditorId + " LVLO size error");
                }
                else
                    reader.get(lvlo);

                reader.adjustFormId(lvlo.item);
                mLvlObject.push_back(lvlo);
                break;
            }
            case ESM4::SUB_LLCT:
            case ESM4::SUB_OBND: // FO3/FONV
            case ESM4::SUB_COED: // FO3/FONV
            case ESM4::SUB_LVLG: // FO3/FONV
            {

                //std::cout << "LVLI " << ESM::printName(subHdr.typeId) << " skipping..." << subHdr.dataSize << std::endl;
                reader.skipSubRecordData();
                break;
            }
            default:
                throw std::runtime_error("ESM4::LVLI::load - Unknown subrecord " + ESM::printName(subHdr.typeId));
        }
    }

    // FIXME: testing
    //if (mHasLvlItemFlags && mChanceNone >= 90)
        //std::cout << "LVLI " << mEditorId << " chance none " << int(mChanceNone) << std::endl;
}

bool ESM4::LevelledItem::calcAllLvlLessThanPlayer() const
{
    if (mHasLvlItemFlags)
        return (mLvlItemFlags & 0x01) != 0;
    else
        return (mChanceNone & 0x80) != 0; // FIXME: 0x80 is just a guess
}

bool ESM4::LevelledItem::calcEachItemInCount() const
{
    if (mHasLvlItemFlags)
        return (mLvlItemFlags & 0x02) != 0;
    else
        return mData != 0;
}

std::int8_t ESM4::LevelledItem::chanceNone() const
{
    if (mHasLvlItemFlags)
        return mChanceNone;
    else
        return (mChanceNone & 0x7f); // FIXME: 0x80 is just a guess
}

bool ESM4::LevelledItem::useAll() const
{
    if (mHasLvlItemFlags)
        return (mLvlItemFlags & 0x04) != 0;
    else
        return false;
}

//void ESM4::LevelledItem::save(ESM4::Writer& writer) const
//{
//}

//void ESM4::LevelledItem::blank()
//{
//}
