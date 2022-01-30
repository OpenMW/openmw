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
#ifndef ESM4_ROAD_H
#define ESM4_ROAD_H

#include <cstdint>
#include <string>
#include <vector>

#include "formid.hpp"

namespace ESM4
{
    class Reader;
    class Writer;

    struct Road
    {
#pragma pack(push, 1)
        // FIXME: duplicated from PGRD
        struct PGRP
        {
            float x;
            float y;
            float z;
            std::uint8_t  numLinks;
            std::uint8_t  unknown1;
            std::uint16_t unknown2;
        };

        // FIXME: duplicated from PGRD
        struct PGRR
        {
            std::int16_t startNode;
            std::int16_t endNode;
        };

        struct RDRP
        {
            float x;
            float y;
            float z;
        };
#pragma pack(pop)
        FormId mParent;       // world FormId, from the loading sequence

        FormId mFormId;       // from the header
        std::uint32_t mFlags; // from the header, see enum type RecordFlag for details

        std::string mEditorId;

        std::vector<PGRP> mNodes;
        std::vector<PGRR> mLinks;

        Road();
        virtual ~Road();

        virtual void load(ESM4::Reader& reader);
        //virtual void save(ESM4::Writer& writer) const;

        //void blank();
    };
}

#endif // ESM4_ROAD_H
