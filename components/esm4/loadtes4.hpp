/*
  Copyright (C) 2015-2016, 2018, 2020-2021 cc9cii

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
#ifndef ESM4_TES4_H
#define ESM4_TES4_H

#include <vector>

#include "formid.hpp"
#include "../esm/common.hpp" // ESMVersion, MasterData

namespace ESM4
{
    class Reader;
    class Writer;

#pragma pack(push, 1)
    struct Data
    {
        ESM::ESMVersion version; // File format version.
        std::int32_t    records; // Number of records
        std::uint32_t   nextObjectId;
    };
#pragma pack(pop)

    struct Header
    {
        std::uint32_t mFlags; // 0x01 esm, 0x80 localised strings

        Data mData;
        std::string mAuthor; // Author's name
        std::string mDesc;   // File description
        std::vector<ESM::MasterData> mMaster;

        std::vector<FormId> mOverrides; // Skyrim only, cell children (ACHR, LAND, NAVM, PGRE, PHZD, REFR)

        // position in the vector = mod index of master files above
        // value = adjusted mod index based on all the files loaded so far
        //std::vector<std::uint32_t> mModIndices;

        void load (Reader& reader);
        //void save (Writer& writer);
    };
}

#endif // ESM4_TES4_H
