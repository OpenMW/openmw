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
#include "loadmset.hpp"

#include <stdexcept>
#include <iostream> // FIXME: for debugging only

#include "reader.hpp"
//#include "writer.hpp"

ESM4::MediaSet::~MediaSet()
{
}

void ESM4::MediaSet::load(ESM4::Reader& reader)
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
            case ESM4::SUB_FULL: reader.getZString(mFullName);  break;
            case ESM4::SUB_NAM1: reader.get(mSetType); break;
            case ESM4::SUB_PNAM: reader.get(mEnabled); break;
            case ESM4::SUB_NAM2: reader.getZString(mSet2); break;
            case ESM4::SUB_NAM3: reader.getZString(mSet3); break;
            case ESM4::SUB_NAM4: reader.getZString(mSet4); break;
            case ESM4::SUB_NAM5: reader.getZString(mSet5); break;
            case ESM4::SUB_NAM6: reader.getZString(mSet6); break;
            case ESM4::SUB_NAM7: reader.getZString(mSet7); break;
            case ESM4::SUB_HNAM: reader.getFormId(mSoundIntro); break;
            case ESM4::SUB_INAM: reader.getFormId(mSoundOutro); break;
            case ESM4::SUB_NAM8: reader.get(mLevel8); break;
            case ESM4::SUB_NAM9: reader.get(mLevel9); break;
            case ESM4::SUB_NAM0: reader.get(mLevel0); break;
            case ESM4::SUB_ANAM: reader.get(mLevelA); break;
            case ESM4::SUB_BNAM: reader.get(mLevelB); break;
            case ESM4::SUB_CNAM: reader.get(mLevelC); break;
            case ESM4::SUB_JNAM: reader.get(mBoundaryDayOuter); break;
            case ESM4::SUB_KNAM: reader.get(mBoundaryDayMiddle); break;
            case ESM4::SUB_LNAM: reader.get(mBoundaryDayInner); break;
            case ESM4::SUB_MNAM: reader.get(mBoundaryNightOuter); break;
            case ESM4::SUB_NNAM: reader.get(mBoundaryNightMiddle); break;
            case ESM4::SUB_ONAM: reader.get(mBoundaryNightInner); break;
            case ESM4::SUB_DNAM: reader.get(mTime1); break;
            case ESM4::SUB_ENAM: reader.get(mTime2); break;
            case ESM4::SUB_FNAM: reader.get(mTime3); break;
            case ESM4::SUB_GNAM: reader.get(mTime4); break;
            case ESM4::SUB_DATA:
            {
                //std::cout << "MSET " << ESM::printName(subHdr.typeId) << " skipping..."
                          //<< subHdr.dataSize << std::endl;
                reader.skipSubRecordData();
                break;
            }
            default:
                throw std::runtime_error("ESM4::MSET::load - Unknown subrecord " + ESM::printName(subHdr.typeId));
        }
    }
}

//void ESM4::MediaSet::save(ESM4::Writer& writer) const
//{
//}

//void ESM4::MediaSet::blank()
//{
//}
