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

#include <cstring>
#include <stdexcept>

#include "reader.hpp"
//#include "writer.hpp"

void ESM4::MediaLocationController::load(ESM4::Reader& reader)
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
                break;
            case ESM::fourCC("FULL"):
                reader.getZString(mFullName);
                break;
            case ESM::fourCC("GNAM"):
                reader.getFormId(mBattleSets.emplace_back());
                break;
            case ESM::fourCC("LNAM"):
                reader.getFormId(mLocationSets.emplace_back());
                break;
            case ESM::fourCC("YNAM"):
                reader.getFormId(mEnemySets.emplace_back());
                break;
            case ESM::fourCC("HNAM"):
                reader.getFormId(mNeutralSets.emplace_back());
                break;
            case ESM::fourCC("XNAM"):
                reader.getFormId(mFriendSets.emplace_back());
                break;
            case ESM::fourCC("ZNAM"):
                reader.getFormId(mAllySets.emplace_back());
                break;
            case ESM::fourCC("RNAM"):
                reader.getFormId(mConditionalFaction);
                break;
            case ESM::fourCC("NAM1"):
            {
                reader.get(mMediaFlags);
                std::uint8_t flags = mMediaFlags.loopingOptions;
                mMediaFlags.loopingOptions = (flags & 0xF0) >> 4;
                mMediaFlags.factionNotFound = flags & 0x0F; // WARN: overwriting data
                break;
            }
            case ESM::fourCC("NAM4"):
                reader.get(mLocationDelay);
                break;
            case ESM::fourCC("NAM7"):
                reader.get(mRetriggerDelay);
                break;
            case ESM::fourCC("NAM5"):
                reader.get(mDayStart);
                break;
            case ESM::fourCC("NAM6"):
                reader.get(mNightStart);
                break;
            case ESM::fourCC("NAM2"): // always 0? 4 bytes
            case ESM::fourCC("NAM3"): // always 0? 4 bytes
            case ESM::fourCC("FNAM"): // always 0? 4 bytes
            {
#if 0
                std::vector<unsigned char> mDataBuf(subHdr.dataSize);
                reader.get(mDataBuf.data(), subHdr.dataSize);

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
                reader.skipSubRecordData();
#endif
                break;
            }
            default:
                throw std::runtime_error("ESM4::ALOC::load - Unknown subrecord " + ESM::printName(subHdr.typeId));
        }
    }
}

// void ESM4::MediaLocationController::save(ESM4::Writer& writer) const
//{
// }

// void ESM4::MediaLocationController::blank()
//{
// }
