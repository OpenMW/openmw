/*
  Copyright (C) 2015-2016, 2018-2019 cc9cii

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
#ifndef ESM4_CELL_H
#define ESM4_CELL_H

#include <cstdint>
#include <string>
#include <vector>

namespace ESM4
{
    class Reader;
    class Writer;
    struct ReaderContext;
    struct CellGroup;
    typedef std::uint32_t FormId;

    enum CellFlags               // TES4                     TES5
    {                            // -----------------------  ------------------------------------
        CELL_Interior = 0x0001,  // Can't travel from here   Interior
        CELL_HasWater = 0x0002,  // Has water                Has Water
        CELL_NoTravel = 0x0004,  //                          not Can't Travel From Here(Int only)
        CELL_HideLand = 0x0008,  // Force hide land (Ext)    No LOD Water
                                 // Oblivion interior (Int)
        CELL_Public   = 0x0020,  // Public place             Public Area
        CELL_HandChgd = 0x0040,  // Hand changed             Hand Changed
        CELL_QuasiExt = 0x0080,  // Behave like exterior     Show Sky
        CELL_SkyLight = 0x0100   //                          Use Sky Lighting
    };

    // Unlike TES3, multiple cells can have the same exterior co-ordinates.
    // The cells need to be organised under world spaces.
    struct Cell
    {
#pragma pack(push, 1)
        // TES4 (guesses only), TES5 are 96 bytes
        struct Lighting
        {                              //               | Aichan Prison values
            std::uint32_t ambient;     //               | 16 17 19 00 (RGBA)
            std::uint32_t directional; //               | 00 00 00 00 (RGBA)
            std::uint32_t fogColor;    //               | 1D 1B 16 00 (RGBA)
            float         fogNear;     // Fog Near      | 00 00 00 00 = 0.f
            float         fogFar;      // Fog Far       | 00 80 3B 45 = 3000.f
            std::int32_t  rotationXY;  // rotation xy   | 00 00 00 00 = 0
            std::int32_t  rotationZ;   // rotation z    | 00 00 00 00 = 0
            float         fogDirFade;  // Fog dir fade  | 00 00 80 3F = 1.f
            float         fogClipDist; // Fog clip dist | 00 80 3B 45 = 3000.f
        };
#pragma pack(pop)

        FormId mParent;       // world formId (for grouping cells), from the loading sequence

        FormId mFormId;       // from the header
        std::uint32_t mFlags; // from the header, see enum type RecordFlag for details

        std::string mEditorId;
        std::string mFullName;
        std::uint16_t mCellFlags; // TES5 can also be 8 bits

        std::int32_t mX;
        std::int32_t mY;

        FormId mOwner;
        FormId mGlobal;
        FormId mClimate;
        FormId mWater;
        float  mWaterHeight;

        std::vector<FormId> mRegions;
        Lighting mLighting;

        CellGroup *mCellGroup;

        Cell();
        virtual ~Cell();

        void init(ESM4::Reader& reader); // common setup for both preload() and load()

        bool mPreloaded;
        bool preload(ESM4::Reader& reader);

        virtual void load(ESM4::Reader& reader);
        //virtual void save(ESM4::Writer& writer) const;

        void blank();
    };
}

#endif // ESM4_CELL_H
