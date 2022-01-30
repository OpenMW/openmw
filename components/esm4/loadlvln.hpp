/*
  Copyright (C) 2019, 2020 cc9cii

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
#ifndef ESM4_LVLN_H
#define ESM4_LVLN_H

#include <cstdint>
#include <vector>

#include "formid.hpp"
#include "inventory.hpp" // LVLO

namespace ESM4
{
    class Reader;
    class Writer;

    struct LevelledNpc
    {
        FormId mFormId;       // from the header
        std::uint32_t mFlags; // from the header, see enum type RecordFlag for details

        std::string mEditorId;
        std::string mModel;

        std::int8_t mChanceNone;
        std::uint8_t mLvlActorFlags;

        std::uint8_t mListCount;
        std::vector<LVLO> mLvlObject;

        LevelledNpc();
        virtual ~LevelledNpc();

        inline bool calcAllLvlLessThanPlayer() const { return (mLvlActorFlags & 0x01) != 0; }
        inline bool calcEachItemInCount() const { return (mLvlActorFlags & 0x02) != 0; }
        inline std::int8_t chanceNone() const { return mChanceNone; }

        virtual void load(ESM4::Reader& reader);
        //virtual void save(ESM4::Writer& writer) const;

        //void blank();
    };
}

#endif // ESM4_LVLN_H
