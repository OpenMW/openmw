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

#include "formid.hpp"

namespace ESM4
{
    class Reader;
    class Writer;

    struct Light
    {
        struct Data
        {
            std::uint32_t time;               // FO/FONV only
            float         duration;
            std::uint32_t radius;
            std::uint32_t colour; // RGBA
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
            std::int32_t  flags;
            float         falloff;
            float         FOV;
            float         nearClip;           // TES5 only
            float         frequency;          // TES5 only
            float         intensityAmplitude; // TES5 only
            float         movementAmplitude;  // TES5 only
            std::uint32_t value;   // gold
            float         weight;
            Data() : duration(-1), radius(0), colour(0), flags(0), falloff(1.f), FOV(90),
                     nearClip(0.f), frequency(0.f), intensityAmplitude(0.f), movementAmplitude(0.f),
                     value(0), weight(0.f) // FIXME: FOV in degrees or radians?
            {}
        };

        FormId mFormId;       // from the header
        std::uint32_t mFlags; // from the header, see enum type RecordFlag for details

        std::string mEditorId;
        std::string mFullName;
        std::string mModel;
        std::string mIcon;

        float mBoundRadius;

        FormId mScriptId;
        FormId mSound;

        float mFade;

        Data mData;

        Light();
        virtual ~Light();

        virtual void load(ESM4::Reader& reader);
        //virtual void save(ESM4::Writer& writer) const;

        //void blank();
    };
}

#endif // ESM4_LIGH_H
