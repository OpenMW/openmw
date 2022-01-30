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
#ifndef ESM4_GRAS_H
#define ESM4_GRAS_H

#include <cstdint>
#include <string>

#include "formid.hpp"

namespace ESM4
{
    class Reader;
    class Writer;

    struct Grass
    {
#pragma pack(push, 1)
        // unused fields are probably packing
        struct Data
        {
            std::uint8_t  density;
            std::uint8_t  minSlope;
            std::uint8_t  maxSlope;
            std::uint8_t  unused;
            std::uint16_t distanceFromWater;
            std::uint16_t unused2;
            /*
            1 Above - At Least
            2 Above - At Most
            3 Below - At Least
            4 Below - At Most
            5 Either - At Least
            6 Either - At Most
            7 Either - At Most Above
            8 Either - At Most Below
            */
            std::uint32_t waterDistApplication;
            float positionRange;
            float heightRange;
            float colorRange;
            float wavePeriod;
            /*
            0x01 Vertex Lighting
            0x02 Uniform Scaling
            0x04 Fit to Slope
            */
            std::uint8_t  flags;
            std::uint8_t  unused3;
            std::uint16_t unused4;
        };
#pragma pack(pop)

        FormId mFormId;       // from the header
        std::uint32_t mFlags; // from the header, see enum type RecordFlag for details

        std::string mEditorId;
        std::string mModel;

        float mBoundRadius;

        Data mData;

        Grass();
        virtual ~Grass();

        virtual void load(ESM4::Reader& reader);
        //virtual void save(ESM4::Writer& writer) const;

        //void blank();
    };
}

#endif // ESM4_GRAS_H
