/*
  Copyright (C) 2016, 2018, 2020 cc9cii

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
#ifndef ESM4_SOUN_H
#define ESM4_SOUN_H

#include <cstdint>
#include <string>

#include "formid.hpp"

namespace ESM4
{
    class Reader;
    class Writer;

    struct Sound
    {
        enum Flags
        {
            Flag_RandomFreqShift = 0x0001,
            Flag_PlayAtRandom    = 0x0002,
            Flag_EnvIgnored      = 0x0004,
            Flag_RandomLocation  = 0x0008,
            Flag_Loop            = 0x0010,
            Flag_MenuSound       = 0x0020,
            Flag_2D              = 0x0040,
            Flag_360LFE          = 0x0080
        };

#pragma pack(push, 1)
        struct SNDX
        {
            std::uint8_t  minAttenuation; // distance?
            std::uint8_t  maxAttenuation; // distance?
            std::int8_t   freqAdjustment; // %, signed
            std::uint8_t  unknown; // probably padding
            std::uint16_t flags;
            std::uint16_t unknown2; // probably padding
            std::uint16_t staticAttenuation; // divide by 100 to get value in dB
            std::uint8_t  stopTime;  // multiply by 1440/256 to get value in minutes
            std::uint8_t  startTime; // multiply by 1440/256 to get value in minutes
        };

        struct SoundData
        {
            std::int16_t attenuationPoint1;
            std::int16_t attenuationPoint2;
            std::int16_t attenuationPoint3;
            std::int16_t attenuationPoint4;
            std::int16_t attenuationPoint5;
            std::int16_t reverbAttenuationControl;
            std::int32_t priority;
            std::int32_t x;
            std::int32_t y;
        };
#pragma pack(pop)

        FormId mFormId;       // from the header
        std::uint32_t mFlags; // from the header, see enum type RecordFlag for details

        std::string mEditorId;

        std::string mSoundFile;
        SNDX mData;
        SoundData mExtra;

        Sound();
        virtual ~Sound();

        virtual void load(ESM4::Reader& reader);
        //virtual void save(ESM4::Writer& writer) const;

        //void blank();
    };
}

#endif // ESM4_SOUN_H
