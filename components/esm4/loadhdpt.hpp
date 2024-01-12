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
#ifndef ESM4_HDPT_H
#define ESM4_HDPT_H

#include <array>
#include <cstdint>
#include <string>

#include <components/esm/defs.hpp>
#include <components/esm/formid.hpp>

namespace ESM4
{
    class Reader;
    class Writer;

    struct HeadPart
    {
        ESM::FormId mId; // from the header
        std::uint32_t mFlags; // from the header, see enum type RecordFlag for details
        std::uint64_t mExtraFlags2;

        std::string mEditorId;
        std::string mFullName;
        std::string mModel;

        std::uint8_t mData;
        std::uint32_t mType;

        enum Type : std::uint32_t
        {
            Type_Misc = 0,
            Type_Face = 1,
            Type_Eyes = 2,
            Type_Hair = 3,
            Type_FacialHair = 4,
            Type_Scar = 5,
            Type_Eyebrows = 6,
            // FO4+
            Type_Meatcaps = 7,
            Type_Teeth = 8,
            Type_HeadRear = 9,
            // Starfield
            // 10 and 11 are unknown
            Type_LeftEye = 12,
            Type_Eyelashes = 13,
        };

        std::vector<ESM::FormId> mExtraParts;

        std::array<std::string, 3> mTriFile;
        ESM::FormId mBaseTexture;
        ESM::FormId mColor;
        std::vector<ESM::FormId> mValidRaces;

        void load(ESM4::Reader& reader);
        // void save(ESM4::Writer& writer) const;

        // void blank();
        static constexpr ESM::RecNameInts sRecordId = ESM::RecNameInts::REC_HDPT4;
    };
}

#endif // ESM4_HDPT_H
