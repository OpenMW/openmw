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
#ifndef ESM4_PACK_H
#define ESM4_PACK_H

#include <cstdint>
#include <string>
#include <vector>

#include "formid.hpp"

namespace ESM4
{
    class Reader;
    class Writer;

    struct AIPackage
    {
#pragma pack(push, 1)
        struct PKDT // data
        {
            std::uint32_t flags;
            std::int32_t  type;
        };

        struct PSDT // schedule
        {
            std::uint8_t month;      // Any = 0xff
            std::uint8_t dayOfWeek;  // Any = 0xff
            std::uint8_t date;       // Any = 0
            std::uint8_t time;       // Any = 0xff
            std::uint32_t duration;
        };

        struct PLDT // location
        {
            std::int32_t type; // 0 = near ref, 1 = in cell, 2 = current loc, 3 = editor loc, 4 = obj id, 5 = obj type
            FormId location;   // uint32_t if type = 5
            std::int32_t radius;
        };

        struct PTDT // target
        {
            std::int32_t type; // 0 = specific ref, 1 = obj id, 2 = obj type
            FormId target;   // uint32_t if type = 2
            std::int32_t distance;
        };

        // NOTE: param1/param2 can be FormId or number, but assume FormId so that adjustFormId
        // can be called
        struct CTDA
        {
            std::uint8_t condition;
            std::uint8_t unknown1; // probably padding
            std::uint8_t unknown2; // probably padding
            std::uint8_t unknown3; // probably padding
            float compValue;
            std::int32_t fnIndex;
            FormId param1;
            FormId param2;
            std::uint32_t unknown4; // probably padding
        };
#pragma pack(pop)

        FormId mFormId;       // from the header
        std::uint32_t mFlags; // from the header, see enum type RecordFlag for details

        std::string mEditorId;

        PKDT mData;
        PSDT mSchedule;
        PLDT mLocation;
        PTDT mTarget;
        std::vector<CTDA> mConditions;

        AIPackage();
        virtual ~AIPackage();

        virtual void load(ESM4::Reader& reader);
        //virtual void save(ESM4::Writer& writer) const;

        //void blank();
    };
}

#endif // ESM4_PACK_H
