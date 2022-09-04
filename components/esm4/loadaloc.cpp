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

*/
#include "loadaloc.hpp"

#include <stdexcept>
#include <cstring>
//#include <iostream> // FIXME: for debugging only
//#include <iomanip>  // FIXME: for debugging only

//#include <boost/scoped_array.hpp> // FIXME

//#include "formid.hpp" // FIXME:

#include "reader.hpp"
//#include "writer.hpp"

void ESM4::MediaLocationController::load(ESM4::Reader& reader)
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
            case ESM4::SUB_GNAM:
            {
                FormId id;
                reader.getFormId(id);
                mBattleSets.push_back(id);

                break;
            }
            case ESM4::SUB_LNAM:
            {
                FormId id;
                reader.getFormId(id);
                mLocationSets.push_back(id);

                break;
            }
            case ESM4::SUB_YNAM:
            {
                FormId id;
                reader.getFormId(id);
                mEnemySets.push_back(id);

                break;
            }
            case ESM4::SUB_HNAM:
            {
                FormId id;
                reader.getFormId(id);
                mNeutralSets.push_back(id);

                break;
            }
            case ESM4::SUB_XNAM:
            {
                FormId id;
                reader.getFormId(id);
                mFriendSets.push_back(id);

                break;
            }
            case ESM4::SUB_ZNAM:
            {
                FormId id;
                reader.getFormId(id);
                mAllySets.push_back(id);

                break;
            }
            case ESM4::SUB_RNAM: reader.getFormId(mConditionalFaction); break;
            case ESM4::SUB_NAM1:
            {
                reader.get(mMediaFlags);
                std::uint8_t flags = mMediaFlags.loopingOptions;
                mMediaFlags.loopingOptions = (flags & 0xF0) >> 4;
                mMediaFlags.factionNotFound = flags & 0x0F; // WARN: overwriting data
                break;
            }
            case ESM4::SUB_NAM4: reader.get(mLocationDelay); break;
            case ESM4::SUB_NAM7: reader.get(mRetriggerDelay); break;
            case ESM4::SUB_NAM5: reader.get(mDayStart); break;
            case ESM4::SUB_NAM6: reader.get(mNightStart); break;
            case ESM4::SUB_NAM2: // always 0? 4 bytes
            case ESM4::SUB_NAM3: // always 0? 4 bytes
            case ESM4::SUB_FNAM: // always 0? 4 bytes
            {
#if 0
                boost::scoped_array<unsigned char> mDataBuf(new unsigned char[subHdr.dataSize]);
                reader.get(mDataBuf.get(), subHdr.dataSize);

                std::ostringstream ss;
                ss << mEditorId << " " << ESM::printName(subHdr.typeId) << ":size " << subHdr.dataSize << "\n";
                for (std::size_t i = 0; i < subHdr.dataSize; ++i)
                {
                    //if (mDataBuf[i] > 64 && mDataBuf[i] < 91) // looks like printable ascii char
                        //ss << (char)(mDataBuf[i]) << " ";
                    //else
                        ss << std::setfill('0') << std::setw(2) << std::hex << (int)(mDataBuf[i]);
                    if ((i & 0x000f) == 0xf) // wrap around
                        ss << "\n";
                    else if (i < subHdr.dataSize-1)
                        ss << " ";
                }
                std::cout << ss.str() << std::endl;
#else
                //std::cout << "ALOC " << ESM::printName(subHdr.typeId) << " skipping..."
                          //<< subHdr.dataSize << std::endl;
                reader.skipSubRecordData();
#endif
                break;
            }
            default:
                //std::cout << "ALOC " << ESM::printName(subHdr.typeId) << " skipping..."
                          //<< subHdr.dataSize << std::endl;
                //reader.skipSubRecordData();
                throw std::runtime_error("ESM4::ALOC::load - Unknown subrecord " + ESM::printName(subHdr.typeId));
        }
    }
}

//void ESM4::MediaLocationController::save(ESM4::Writer& writer) const
//{
//}

//void ESM4::MediaLocationController::blank()
//{
//}
