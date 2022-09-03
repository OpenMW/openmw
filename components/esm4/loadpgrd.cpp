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
#include "loadpgrd.hpp"

#include <stdexcept>
//#include <iostream> // FIXME: for debugging only
//#include <iomanip>  // FIXME: for debugging only

//#include <boost/scoped_array.hpp> // FIXME for debugging only

#include "formid.hpp" // FIXME: for mEditorId workaround
#include "reader.hpp"
//#include "writer.hpp"

void ESM4::Pathgrid::load(ESM4::Reader& reader)
{
    mFormId = reader.hdr().record.id;
    reader.adjustFormId(mFormId);
    mFlags  = reader.hdr().record.flags;

    mEditorId = formIdToString(mFormId); // FIXME: quick workaround to use existing code

    while (reader.getSubRecordHeader())
    {
        const ESM4::SubRecordHeader& subHdr = reader.subRecordHeader();
        switch (subHdr.typeId)
        {
            case ESM4::SUB_DATA: reader.get(mData); break;
            case ESM4::SUB_PGRP:
            {
                std::size_t numNodes = subHdr.dataSize / sizeof(PGRP);
                if (numNodes != std::size_t(mData)) // keep gcc quiet
                    throw std::runtime_error("ESM4::PGRD::load numNodes mismatch");

                mNodes.resize(numNodes);
                for (std::size_t i = 0; i < numNodes; ++i)
                {
                    reader.get(mNodes.at(i));

                    if (int(mNodes.at(i).z) % 2 == 0)
                        mNodes.at(i).priority = 0;
                    else
                        mNodes.at(i).priority = 1;
                }

                break;
            }
            case ESM4::SUB_PGRR:
            {
                static PGRR link;

                for (std::size_t i = 0; i < std::size_t(mData); ++i) // keep gcc quiet
                {
                    for (std::size_t j = 0; j < mNodes[i].numLinks; ++j)
                    {
                        link.startNode = std::int16_t(i);

                        reader.get(link.endNode);
                        if (link.endNode == -1)
                            continue;

                        // ICMarketDistrictTheBestDefenseBasement doesn't have a PGRR sub-record
                        // CELL formId 00049E2A
                        // PGRD formId 000304B7
                        //if (mFormId == 0x0001C2C8)
                            //std::cout << link.startNode << "," << link.endNode << std::endl;
                        mLinks.push_back(link);
                    }
                }

                break;
            }
            case ESM4::SUB_PGRI:
            {
                std::size_t numForeign = subHdr.dataSize / sizeof(PGRI);
                mForeign.resize(numForeign);
                for (std::size_t i = 0; i < numForeign; ++i)
                {
                    reader.get(mForeign.at(i));
                    // mForeign.at(i).localNode;// &= 0xffff; // some have junk high bits (maybe flags?)
                }

                break;
            }
            case ESM4::SUB_PGRL:
            {
                static PGRL objLink;
                reader.get(objLink.object);
                //                                        object             linkedNode
                std::size_t numNodes = (subHdr.dataSize - sizeof(int32_t)) / sizeof(int32_t);

                objLink.linkedNodes.resize(numNodes);
                for (std::size_t i = 0; i < numNodes; ++i)
                    reader.get(objLink.linkedNodes.at(i));

                mObjects.push_back(objLink);

                break;
            }
            case ESM4::SUB_PGAG:
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
                //std::cout << "PGRD " << ESM::printName(subHdr.typeId) << " skipping..."
                        //<< subHdr.dataSize << std::endl;
                reader.skipSubRecordData();
#endif
                break;
            }
            default:
                throw std::runtime_error("ESM4::PGRD::load - Unknown subrecord " + ESM::printName(subHdr.typeId));
        }
    }
}

//void ESM4::Pathgrid::save(ESM4::Writer& writer) const
//{
//}

//void ESM4::Pathgrid::blank()
//{
//}
