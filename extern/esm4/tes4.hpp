/*
  Copyright (C) 2015-2016, 2018 cc9cii

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

#include <string>
#include <cstdint>
#include <vector>

namespace ESM4
{
    class Reader;
    class Writer;
    typedef std::uint32_t FormId;

    struct Header
    {
#pragma pack(push, 1)
        union ESMVersion
        {
            float        f;
            unsigned int ui;
        };

        struct Data
        {
            // The supported versions are 0.80 = 0x3f800000, 0.94 = 0x3f70a3d7 and 1.7 = 0x3fd9999a
            // (also 1.34 = 0x3fab851f eventually)
            ESMVersion    version; // File format version.
            std::int32_t  records; // Number of records
            std::uint32_t nextObjectId;
        };
#pragma pack(pop)

        // Defines another files (esm or esp) that this file depends upon.
        struct MasterData
        {
            std::string   name;
            std::uint64_t size;
        };

        std::uint32_t mFlags; // 0x01 esm, 0x80 localised strings

        Data mData;
        std::string mAuthor; // Author's name
        std::string mDesc;   // File description
        std::vector<MasterData> mMaster;

        std::vector<FormId> mOverrides; // Skyrim only, cell children (ACHR, LAND, NAVM, PGRE, PHZD, REFR)

        // position in the vector = mod index of master files above
        // value = adjusted mod index based on all the files loaded so far
        std::vector<std::uint32_t> mModIndicies;

        void load (Reader& reader);
        //void save (Writer& writer);
    };
}

#endif // ESM4_TES4_H
