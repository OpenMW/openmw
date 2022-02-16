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
#ifndef ESM4_MSET_H
#define ESM4_MSET_H

#include <cstdint>
#include <string>

#include "formid.hpp"

namespace ESM4
{
    class Reader;
    class Writer;

    struct MediaSet
    {
        FormId mFormId;       // from the header
        std::uint32_t mFlags; // from the header, see enum type RecordFlag for details

        std::string mEditorId;
        std::string mFullName;

        // -1 none, 0 battle, 1 location, 2 dungeon, 3 incidental
        // Battle     - intro (HNAM), loop (NAM2), outro (INAM)
        // Location   - day outer (NAM2), day middle (NAM3), day inner (NAM4),
        //              night outer (NAM5), night middle (NAM6), night inner (NAM7)
        // Dungeon    - intro (HNAM), battle (NAM2), explore (NAM3), suspence (NAM4), outro (INAM)
        // Incidental - daytime (HNAM), nighttime (INAM)
        std::int32_t mSetType;
        // 0x01 day outer,   0x02 day middle,   0x04 day inner
        // 0x08 night outer, 0x10 night middle, 0x20 night inner
        std::uint8_t mEnabled; // for location

        float mBoundaryDayOuter;    // %
        float mBoundaryDayMiddle;   // %
        float mBoundaryDayInner;    // %
        float mBoundaryNightOuter;  // %
        float mBoundaryNightMiddle; // %
        float mBoundaryNightInner;  // %

        // start at 2 to reduce confusion
        std::string mSet2; // NAM2
        std::string mSet3; // NAM3
        std::string mSet4; // NAM4
        std::string mSet5; // NAM5
        std::string mSet6; // NAM6
        std::string mSet7; // NAM7

        float mLevel8; // dB
        float mLevel9; // dB
        float mLevel0; // dB
        float mLevelA; // dB
        float mLevelB; // dB
        float mLevelC; // dB

        float mTime1;
        float mTime2;
        float mTime3;
        float mTime4;

        FormId mSoundIntro; // HNAM
        FormId mSoundOutro; // INAM

        MediaSet();
        virtual ~MediaSet();

        virtual void load(ESM4::Reader& reader);
        //virtual void save(ESM4::Writer& writer) const;

        //void blank();
    };
}

#endif // ESM4_MSET_H
