/*
  Copyright (C) 2020 - 2021 cc9cii

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
#include "loadroad.hpp"

#include <stdexcept>

#include "reader.hpp"
#include <components/esm/refid.hpp> // FIXME: for workaround
//#include "writer.hpp"

void ESM4::Road::load(ESM4::Reader& reader)
{
    mId = reader.getFormIdFromHeader();
    mFlags = reader.hdr().record.flags;
    mParent = reader.currWorld();

    mEditorId = ESM::RefId(mId).serializeText(); // FIXME: quick workaround to use existing code

    while (reader.getSubRecordHeader())
    {
        const ESM4::SubRecordHeader& subHdr = reader.subRecordHeader();
        switch (subHdr.typeId)
        {
            case ESM::fourCC("PGRP"):
            {
                std::size_t numNodes = subHdr.dataSize / sizeof(PGRP);

                mNodes.resize(numNodes);
                for (std::size_t i = 0; i < numNodes; ++i)
                {
                    reader.get(mNodes.at(i));
                }

                break;
            }
            case ESM::fourCC("PGRR"):
            {
                PGRR link;
                RDRP linkPt;

                for (std::size_t i = 0; i < mNodes.size(); ++i)
                {
                    for (std::size_t j = 0; j < mNodes[i].numLinks; ++j)
                    {
                        link.startNode = std::int16_t(i);

                        reader.get(linkPt);

                        // FIXME: instead of looping each time, maybe use a map?
                        bool found = false;
                        for (std::size_t k = 0; k < mNodes.size(); ++k)
                        {
                            if (linkPt.x != mNodes[k].x || linkPt.y != mNodes[k].y || linkPt.z != mNodes[k].z)
                                continue;
                            else
                            {
                                link.endNode = std::int16_t(k);
                                mLinks.push_back(link);

                                found = true;
                                break;
                            }
                        }

                        if (!found)
                            throw std::runtime_error("ESM4::ROAD::PGRR - Unknown link point " + std::to_string(j)
                                + "at node " + std::to_string(i) + ".");
                    }
                }

                break;
            }
            default:
                throw std::runtime_error("ESM4::ROAD::load - Unknown subrecord " + ESM::printName(subHdr.typeId));
        }
    }
}

// void ESM4::Road::save(ESM4::Writer& writer) const
//{
// }

// void ESM4::Road::blank()
//{
// }
