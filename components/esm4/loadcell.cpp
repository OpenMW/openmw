/*
  Copyright (C) 2015-2016, 2018-2021 cc9cii

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
#include "loadcell.hpp"

#include <cfloat> // FLT_MAX for gcc
#include <limits>
#include <stdexcept>

#include "grouptype.hpp"
#include "reader.hpp"
// #include "writer.hpp"

#include <components/esm/refid.hpp>

float ESM4::Cell::sInvalidWaterLevel = -200000.f;

// TODO: Try loading only EDID and XCLC (along with mFormId, mFlags and mParent)
//
// But, for external cells we may be scanning the whole record since we don't know if there is
// going to be an EDID subrecord. And the vast majority of cells are these kinds.
//
// So perhaps some testing needs to be done to see if scanning and skipping takes
// longer/shorter/same as loading the subrecords.
void ESM4::Cell::load(ESM4::Reader& reader)
{
    ESM::FormId formId = reader.getFormIdFromHeader();
    mId = formId;
    mFlags = reader.hdr().record.flags;
    mParent = reader.currWorld();
    mWaterHeight = sInvalidWaterLevel;
    reader.clearCellGrid(); // clear until XCLC FIXME: somehow do this automatically?

    // Sometimes cell 0,0 does not have an XCLC sub record (e.g. ToddLand 000009BF)
    // To workaround this issue put a default value if group is "exterior sub cell" and its
    // grid from label is "0 0".  Note the reversed X/Y order (no matter since they're both 0
    // anyway).
    if (reader.grp().type == ESM4::Grp_ExteriorSubCell && reader.grp().label.grid[1] == 0
        && reader.grp().label.grid[0] == 0)
    {
        ESM4::CellGrid currCellGrid;
        currCellGrid = Grid{ 0, 0 };
        reader.setCurrCellGrid(currCellGrid); // side effect: sets mCellGridValid  true
    }

    // WARN: we need to call setCurrCell (and maybe setCurrCellGrid?) again before loading
    // cell child groups if we are loading them after restoring the context
    // (may be easier to update the context before saving?)
    reader.setCurrCell(formId); // save for LAND (and other children) to access later
    std::uint32_t esmVer = reader.esmVersion();
    bool isSkyrim = (esmVer == ESM::VER_170 || esmVer == ESM::VER_094);

    while (reader.getSubRecordHeader())
    {
        const ESM4::SubRecordHeader& subHdr = reader.subRecordHeader();
        switch (subHdr.typeId)
        {
            case ESM4::SUB_EDID:
            {
                if (!reader.getZString(mEditorId))
                    throw std::runtime_error("CELL EDID data read error");
#if 0
                std::string padding;
                padding.insert(0, reader.stackSize()*2, ' ');
                std::cout << padding << "CELL Editor ID: " << mEditorId << std::endl;
#endif
                break;
            }
            case ESM4::SUB_XCLC:
            {
                //(X, Y) grid location of the cell followed by flags. Always in
                // exterior cells and never in interior cells.
                //
                //    int32 - X
                //    int32 - Y
                //    uint32 - flags (high bits look random)
                //
                //        0x1 - Force Hide Land Quad 1
                //        0x2 - Force Hide Land Quad 2
                //        0x4 - Force Hide Land Quad 3
                //        0x8 - Force Hide Land Quad 4
                uint32_t flags;
                reader.get(mX);
                reader.get(mY);
                if (subHdr.dataSize == 12)
                    reader.get(flags);

                // Remember cell grid for later (loading LAND, NAVM which should be CELL temporary children)
                // Note that grids only apply for external cells.  For interior cells use the cell's formid.
                reader.setCurrCellGrid(Grid{ static_cast<int16_t>(mX), static_cast<int16_t>(mY) });

                break;
            }
            case ESM4::SUB_FULL:
                reader.getLocalizedString(mFullName);
                break;
            case ESM4::SUB_DATA:
            {
                if (subHdr.dataSize == 2)
                    reader.get(mCellFlags);
                else
                {
                    if (subHdr.dataSize != 1)
                        throw std::runtime_error("CELL unexpected DATA flag size");
                    std::uint8_t value = 0;
                    reader.get(value);
                    mCellFlags = value;
                }
                break;
            }
            case ESM4::SUB_XCLR: // for exterior cells
            {
                mRegions.resize(subHdr.dataSize / sizeof(ESM::FormId32));
                for (std::vector<ESM::FormId>::iterator it = mRegions.begin(); it != mRegions.end(); ++it)
                {
                    reader.getFormId(*it);
#if 0
                    std::string padding;
                    padding.insert(0, reader.stackSize()*2, ' ');
                    std::cout << padding  << "region: " << std::hex << *it << std::endl;
#endif
                }
                break;
            }
            case ESM4::SUB_XOWN:
            {
                switch (subHdr.dataSize)
                {
                    case 4:
                        reader.getFormId(mOwner);
                        break;
                    case 12:
                    {
                        reader.getFormId(mOwner);
                        std::uint32_t dummy;
                        reader.get(dummy); // Unknown
                        reader.get(dummy); // No crime flag, FO4
                        break;
                    }
                    default:
                        reader.skipSubRecordData();
                        break;
                }
                break;
            }
            case ESM4::SUB_XGLB:
                reader.getFormId(mGlobal);
                break; // Oblivion only?
            case ESM4::SUB_XCCM:
                reader.getFormId(mClimate);
                break;
            case ESM4::SUB_XCWT:
                reader.getFormId(mWater);
                break;
            case ESM4::SUB_XCLW:
                reader.get(mWaterHeight);
                break;
            case ESM4::SUB_XCLL:
            {
                switch (subHdr.dataSize)
                {
                    case 36: // TES4
                        reader.get(&mLighting, 36);
                        break;
                    case 40: // FO3/FNV
                    case 92: // TES5 (FIXME)
                    case 136: // FO4 (FIXME)
                        reader.get(mLighting);
                        reader.skipSubRecordData(subHdr.dataSize - 40);
                        break;
                    default:
                        reader.skipSubRecordData();
                        break;
                }
                break;
            }
            case ESM4::SUB_XCMT:
                reader.get(mMusicType);
                break; // Oblivion only?
            case ESM4::SUB_LTMP:
                reader.getFormId(mLightingTemplate);
                break;
            case ESM4::SUB_LNAM:
                reader.get(mLightingTemplateFlags);
                break; // seems to always follow LTMP
            case ESM4::SUB_XCMO:
                reader.getFormId(mMusic);
                break;
            case ESM4::SUB_XCAS:
                reader.getFormId(mAcousticSpace);
                break;
            case ESM4::SUB_TVDT:
            case ESM4::SUB_MHDT:
            case ESM4::SUB_XCGD:
            case ESM4::SUB_XNAM:
            case ESM4::SUB_XLCN:
            case ESM4::SUB_XWCS:
            case ESM4::SUB_XWCU:
            case ESM4::SUB_XWCN:
            case ESM4::SUB_XCIM:
            case ESM4::SUB_XEZN:
            case ESM4::SUB_XWEM:
            case ESM4::SUB_XILL:
            case ESM4::SUB_XRNK:
            case ESM4::SUB_XCET: // FO3
            case ESM4::SUB_IMPF: // FO3 Zeta
            case ESM4::SUB_CNAM: // FO4
            case ESM4::SUB_PCMB: // FO4
            case ESM4::SUB_RVIS: // FO4
            case ESM4::SUB_VISI: // FO4
            case ESM4::SUB_XGDR: // FO4
            case ESM4::SUB_XILW: // FO4
            case ESM4::SUB_XCRI: // FO4
            case ESM4::SUB_XPRI: // FO4
            case ESM4::SUB_ZNAM: // FO4
                reader.skipSubRecordData();
                break;
            default:
                throw std::runtime_error("ESM4::CELL::load - Unknown subrecord " + ESM::printName(subHdr.typeId));
        }
    }
    if (isSkyrim) // Skyrim seems to have broken water level records. But the subrecord exists so it
                  // shouldn't be skipped.
    {
        mWaterHeight = sInvalidWaterLevel;
    }
    mReaderContext = reader.getContext();
}

// void ESM4::Cell::save(ESM4::Writer& writer) const
//{
// }

void ESM4::Cell::blank() {}
