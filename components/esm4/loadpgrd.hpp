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
#ifndef ESM4_PGRD_H
#define ESM4_PGRD_H

#include <cstdint>
#include <string>
#include <vector>

#include "formid.hpp"

namespace ESM4
{
    class Reader;
    class Writer;

    struct Pathgrid
    {
#pragma pack(push, 1)
        struct PGRP
        {
            float x;
            float y;
            float z;
            std::uint8_t  numLinks;
            std::uint8_t  priority; // probably padding, repurposing
            std::uint16_t unknown;  // probably padding
        };

        struct PGRR
        {
            std::int16_t startNode;
            std::int16_t endNode;
        };

        struct PGRI
        {
            std::int32_t localNode;
            float x; // foreign
            float y; // foreign
            float z; // foreign
        };
#pragma pack(pop)

        struct PGRL
        {
            FormId object;
            std::vector<std::int32_t> linkedNodes;
        };

        FormId mFormId;       // from the header
        std::uint32_t mFlags; // from the header, see enum type RecordFlag for details

        std::string mEditorId; // FIXME: no such record for PGRD, but keep here to avoid extra work for now

        std::int16_t mData; // number of nodes
        std::vector<PGRP> mNodes;
        std::vector<PGRR> mLinks;
        std::vector<PGRI> mForeign;
        std::vector<PGRL> mObjects;

        Pathgrid();
        virtual ~Pathgrid();

        virtual void load(ESM4::Reader& reader);
        //virtual void save(ESM4::Writer& writer) const;

        //void blank();
    };
}

#endif // ESM4_PGRD_H
