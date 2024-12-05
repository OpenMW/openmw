/*
  Copyright (C) 2016, 2018-2020 cc9cii

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
#ifndef ESM4_LIGH_H
#define ESM4_LIGH_H

#include <cstdint>
#include <string>

#include <components/esm/defs.hpp>
#include <components/esm/formid.hpp>

namespace ESM4
{
    class Reader;
    class Writer;

    struct Light
    {
        enum Flag
        {
            Dynamic = 0x01,
            Carryable = 0x02,
            Negative = 0x04,
            Flicker = 0x08,
            OffDefault = 0x020,
            FlickerSlow = 0x040,
            Pulse = 0x080,
            PulseSlow = 0x100,
            SpotLight = 0x200,
            SpotShadow = 0x400,
        };

        struct Data
        {
            std::int32_t time = 0;
            std::uint32_t radius = 0;
            std::uint32_t colour = 0; // RGBA
            // flags:
            // 0x00000001 = Dynamic
            // 0x00000002 = Can be Carried
            // 0x00000004 = Negative
            // 0x00000008 = Flicker
            // 0x00000020 = Off By Default
            // 0x00000040 = Flicker Slow
            // 0x00000080 = Pulse
            // 0x00000100 = Pulse Slow
            // 0x00000200 = Spot Light
            // 0x00000400 = Spot Shadow
            std::int32_t flags = 0;
            float falloff = 1.f;
            float FOV = 90; // FIXME: FOV in degrees or radians?
            float nearClip = 0.f; // TES5+
            float frequency = 0.f; // TES5+
            float intensityAmplitude = 0.f; // TES5+
            float movementAmplitude = 0.f; // TES5+
            float constant = 0.f; // FO4
            float scalar = 0.f; // FO4
            float exponent = 0.f; // FO4
            float godRaysNearClip = 0.f; // FO4
            std::uint32_t value = 0; // gold
            float weight = 0.f;
        };

        ESM::FormId mId; // from the header
        std::uint32_t mFlags; // from the header, see enum type RecordFlag for details

        std::string mEditorId;
        std::string mFullName;
        std::string mModel;
        std::string mIcon;

        float mBoundRadius;

        ESM::FormId mScriptId;
        ESM::FormId mSound;

        float mFade;

        Data mData;

        void load(ESM4::Reader& reader);
        // void save(ESM4::Writer& writer) const;

        static constexpr ESM::RecNameInts sRecordId = ESM::REC_LIGH4;

        // void blank();
    };
}

#endif // ESM4_LIGH_H
