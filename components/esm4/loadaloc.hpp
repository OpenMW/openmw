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
#ifndef ESM4_ALOC_H
#define ESM4_ALOC_H

#include <cstdint>
#include <string>
#include <vector>

#include "formid.hpp"

namespace ESM4
{
    class Reader;
    class Writer;

#pragma pack(push, 1)
    struct MLC_Flags
    {
        // use day/night transition:  0 = loop, 1 = random, 2 = retrigger, 3 = none
        // use defaults (6:00/23:54): 4 = loop, 5 = random, 6 = retrigger, 7 = none
        std::uint8_t loopingOptions;
        // 0 = neutral, 1 = enemy, 2 = ally, 3 = friend, 4 = location, 5 = none
        std::uint8_t factionNotFound; // WARN: overwriting whatever is in this
        std::uint16_t unknown; // padding?
    };
#pragma pack(pop)

    struct MediaLocationController
    {
        FormId mFormId;       // from the header
        std::uint32_t mFlags; // from the header, see enum type RecordFlag for details

        std::string mEditorId;
        std::string mFullName;

        std::vector<FormId> mBattleSets;
        std::vector<FormId> mLocationSets;
        std::vector<FormId> mEnemySets;
        std::vector<FormId> mNeutralSets;
        std::vector<FormId> mFriendSets;
        std::vector<FormId> mAllySets;

        MLC_Flags mMediaFlags;

        FormId mConditionalFaction;

        float mLocationDelay;
        float mRetriggerDelay;

        std::uint32_t mDayStart;
        std::uint32_t mNightStart;

        MediaLocationController();
        virtual ~MediaLocationController();

        virtual void load(ESM4::Reader& reader);
        //virtual void save(ESM4::Writer& writer) const;

        //void blank();
    };
}

#endif // ESM4_ALOC_H
