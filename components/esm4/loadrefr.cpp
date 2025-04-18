/*
  Copyright (C) 2015-2016, 2018, 2020-2021 cc9cii

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
#include "loadrefr.hpp"

#include <stdexcept>

#include "reader.hpp"
// #include "writer.hpp"

void ESM4::Reference::load(ESM4::Reader& reader)
{
    mId = reader.hdr().record.getFormId();
    reader.adjustFormId(mId);
    mFlags = reader.hdr().record.flags;
    mParent = reader.currCell();

    ESM::FormId mid;
    ESM::FormId sid;

    while (reader.getSubRecordHeader())
    {
        const ESM4::SubRecordHeader& subHdr = reader.subRecordHeader();
        switch (subHdr.typeId)
        {
            case ESM::fourCC("EDID"):
                reader.getZString(mEditorId);
                break;
            case ESM::fourCC("FULL"):
                reader.getLocalizedString(mFullName);
                break;
            case ESM::fourCC("NAME"):
            {
                ESM::FormId baseId;
                reader.getFormId(baseId);
                mBaseObj = baseId;
                break;
            }
            case ESM::fourCC("DATA"):
                reader.get(mPos);
                break;
            case ESM::fourCC("XSCL"):
                reader.get(mScale);
                break;
            case ESM::fourCC("XOWN"):
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
            case ESM::fourCC("XGLB"):
                reader.getFormId(mGlobal);
                break;
            case ESM::fourCC("XRNK"):
                reader.get(mFactionRank);
                break;
            case ESM::fourCC("XESP"):
            {
                reader.getFormId(mEsp.parent);
                reader.get(mEsp.flags);
                // std::cout << "REFR  parent: " << formIdToString(mEsp.parent) << " ref " << formIdToString(mFormId)
                // << ", 0x" << std::hex << (mEsp.flags & 0xff) << std::endl;// FIXME
                break;
            }
            case ESM::fourCC("XTEL"):
            {
                switch (subHdr.dataSize)
                {
                    case 28:
                    case 32: // FO3, FNV, TES5
                    case 36: // FO4
                    {
                        reader.getFormId(mDoor.destDoor);
                        reader.get(mDoor.destPos);
                        mDoor.flags = 0;
                        if (subHdr.dataSize >= 32)
                        {
                            reader.get(mDoor.flags);
                            if (subHdr.dataSize == 36)
                                reader.getFormId(mDoor.transitionInterior);
                        }
                        break;
                    }
                    default:
                        reader.skipSubRecordData();
                        break;
                }
                break;
            }
            case ESM::fourCC("XSED"):
            {
                // 1 or 4 bytes
                if (subHdr.dataSize == 1)
                {
                    uint8_t data;
                    reader.get(data);
                    // std::cout << "REFR XSED " << std::hex << (int)data << std::endl;
                    break;
                }
                else if (subHdr.dataSize == 4)
                {
                    uint32_t data;
                    reader.get(data);
                    // std::cout << "REFR XSED " << std::hex << (int)data << std::endl;
                    break;
                }

                // std::cout << "REFR XSED dataSize: " << subHdr.dataSize << std::endl;// FIXME
                reader.skipSubRecordData();
                break;
            }
            case ESM::fourCC("XLOD"):
            {
                // 12 bytes
                if (subHdr.dataSize == 12)
                {
                    float data, data2, data3;
                    reader.get(data);
                    reader.get(data2);
                    reader.get(data3);
                    // bool hasVisibleWhenDistantFlag = (mFlags & 0x00008000) != 0; // currently unused
                    //  some are trees, e.g. 000E03B6, mBaseObj 00022F32, persistent, visible when distant
                    //  some are doors, e.g. 000270F7, mBaseObj 000CD338, persistent, initially disabled
                    //  (this particular one is an Oblivion Gate)
                    // std::cout << "REFR XLOD " << std::hex << (int)data << " " << (int)data2 << " " << (int)data3 <<
                    // std::endl;
                    break;
                }
                // std::cout << "REFR XLOD dataSize: " << subHdr.dataSize << std::endl;// FIXME
                reader.skipSubRecordData();
                break;
            }
            case ESM::fourCC("XACT"):
            {
                if (subHdr.dataSize == 4)
                {
                    uint32_t data;
                    reader.get(data);
                    // std::cout << "REFR XACT " << std::hex << (int)data << std::endl;
                    break;
                }

                // std::cout << "REFR XACT dataSize: " << subHdr.dataSize << std::endl;// FIXME
                reader.skipSubRecordData();
                break;
            }
            case ESM::fourCC("XRTM"): // formId
            {
                // seems like another ref, e.g. 00064583 has base object 00000034 which is "XMarkerHeading"
                // e.g. some are doors (prob. quest related)
                //    MS94OblivionGateRef XRTM : 00097C88
                //    MQ11SkingradGate    XRTM : 00064583
                //    MQ11ChorrolGate     XRTM : 00188770
                //    MQ11LeyawiinGate    XRTM : 0018AD7C
                //    MQ11AnvilGate       XRTM : 0018D452
                //    MQ11BravilGate      XRTM : 0018AE1B
                // e.g. some are XMarkerHeading
                //    XRTM : 000A4DD7 in OblivionRDCavesMiddleA05 (maybe spawn points?)
                ESM::FormId marker;
                reader.getFormId(marker);
                // std::cout << "REFR " << mEditorId << " XRTM : " << formIdToString(marker) << std::endl;// FIXME
                break;
            }
            case ESM::fourCC("TNAM"): // reader.get(mMapMarker); break;
            {
                if (subHdr.dataSize != sizeof(mMapMarker))
                    // reader.skipSubRecordData(); // FIXME: FO3
                    reader.getFormId(mid);
                else
                    reader.get(mMapMarker); // TES4

                break;
            }
            case ESM::fourCC("XMRK"):
                mIsMapMarker = true;
                break; // all have mBaseObj 0x00000010 "MapMarker"
            case ESM::fourCC("FNAM"):
            {
                // std::cout << "REFR " << ESM::printName(subHdr.typeId) << " skipping..."
                // << subHdr.dataSize << std::endl;
                reader.skipSubRecordData();
                break;
            }
            case ESM::fourCC("XTRG"): // formId
            {
                reader.getFormId(mTargetRef);
                // std::cout << "REFR XRTG : " << formIdToString(id) << std::endl;// FIXME
                break;
            }
            case ESM::fourCC("CNAM"):
                reader.getFormId(mAudioLocation);
                break; // FONV
            case ESM::fourCC("XRDO"): // FO3
            {
                // FIXME: completely different meaning in FO4
                reader.get(mRadio.rangeRadius);
                reader.get(mRadio.broadcastRange);
                reader.get(mRadio.staticPercentage);
                reader.getFormId(mRadio.posReference);

                break;
            }
            case ESM::fourCC("SCRO"): // FO3
            {
                reader.getFormId(sid);
                // if (mFormId == 0x0016b74B)
                // std::cout << "REFR SCRO : " << formIdToString(sid) << std::endl;// FIXME
                break;
            }
            case ESM::fourCC("XLOC"):
            {
                mIsLocked = true;
                std::int8_t dummy; // FIXME: very poor code

                reader.get(mLockLevel);
                reader.get(dummy);
                reader.get(dummy);
                reader.get(dummy);
                ESM::FormId keyForm;
                reader.getFormId(keyForm);
                mKey = keyForm;
                reader.get(dummy); // flag?
                reader.get(dummy);
                reader.get(dummy);
                reader.get(dummy);
                if (subHdr.dataSize == 16)
                    reader.skipSubRecordData(4); // Oblivion (sometimes), flag?
                else if (subHdr.dataSize == 20) // Skyrim, FO3
                    reader.skipSubRecordData(8); // flag?

                break;
            }
            case ESM::fourCC("XCNT"):
            {
                reader.get(mCount);
                break;
            }
            // lighting
            case ESM::fourCC("LNAM"): // lighting template formId
            case ESM::fourCC("XLIG"): // struct, FOV, fade, etc
            case ESM::fourCC("XEMI"): // LIGH formId
            case ESM::fourCC("XRDS"): // Radius or Radiance
            case ESM::fourCC("XRGB"):
            case ESM::fourCC("XRGD"): // tangent data?
            case ESM::fourCC("XALP"): // alpha cutoff
            //
            case ESM::fourCC("XPCI"): // formId
            case ESM::fourCC("XLCM"):
            case ESM::fourCC("ONAM"):
            case ESM::fourCC("VMAD"):
            case ESM::fourCC("XPRM"):
            case ESM::fourCC("INAM"):
            case ESM::fourCC("PDTO"):
            case ESM::fourCC("SCHR"):
            case ESM::fourCC("SCTX"):
            case ESM::fourCC("XAPD"):
            case ESM::fourCC("XAPR"):
            case ESM::fourCC("XCVL"):
            case ESM::fourCC("XCZA"):
            case ESM::fourCC("XCZC"):
            case ESM::fourCC("XEZN"):
            case ESM::fourCC("XFVC"):
            case ESM::fourCC("XHTW"):
            case ESM::fourCC("XIS2"):
            case ESM::fourCC("XLCN"):
            case ESM::fourCC("XLIB"):
            case ESM::fourCC("XLKR"):
            case ESM::fourCC("XLRM"):
            case ESM::fourCC("XLRT"):
            case ESM::fourCC("XLTW"):
            case ESM::fourCC("XMBO"):
            case ESM::fourCC("XMBP"):
            case ESM::fourCC("XMBR"):
            case ESM::fourCC("XNDP"):
            case ESM::fourCC("XOCP"):
            case ESM::fourCC("XPOD"):
            case ESM::fourCC("XPTL"):
            case ESM::fourCC("XPPA"):
            case ESM::fourCC("XPRD"):
            case ESM::fourCC("XPWR"):
            case ESM::fourCC("XRMR"):
            case ESM::fourCC("XSPC"):
            case ESM::fourCC("XTNM"):
            case ESM::fourCC("XTRI"):
            case ESM::fourCC("XWCN"):
            case ESM::fourCC("XWCU"):
            case ESM::fourCC("XATR"):
            case ESM::fourCC("XHLT"): // Unofficial Oblivion Patch
            case ESM::fourCC("XCHG"): // thievery.exp
            case ESM::fourCC("XHLP"): // FO3
            case ESM::fourCC("XAMT"): // FO3
            case ESM::fourCC("XAMC"): // FO3
            case ESM::fourCC("XRAD"): // FO3
            case ESM::fourCC("XIBS"): // FO3
            case ESM::fourCC("XORD"): // FO3
            case ESM::fourCC("XCLP"): // FO3
            case ESM::fourCC("SCDA"): // FO3
            case ESM::fourCC("RCLR"): // FO3
            case ESM::fourCC("BNAM"): // FONV
            case ESM::fourCC("MMRK"): // FONV
            case ESM::fourCC("MNAM"): // FONV
            case ESM::fourCC("NNAM"): // FONV
            case ESM::fourCC("XATO"): // FONV
            case ESM::fourCC("SCRV"): // FONV
            case ESM::fourCC("SCVR"): // FONV
            case ESM::fourCC("SLSD"): // FONV
            case ESM::fourCC("XSRF"): // FONV
            case ESM::fourCC("XSRD"): // FONV
            case ESM::fourCC("WMI1"): // FONV
            case ESM::fourCC("XLRL"): // Unofficial Skyrim Patch
            case ESM::fourCC("XASP"): // FO4
            case ESM::fourCC("XATP"): // FO4
            case ESM::fourCC("XBSD"): // FO4
            case ESM::fourCC("XCVR"): // FO4
            case ESM::fourCC("XCZR"): // FO4
            case ESM::fourCC("XLKT"): // FO4
            case ESM::fourCC("XLYR"): // FO4
            case ESM::fourCC("XMSP"): // FO4
            case ESM::fourCC("XPDD"): // FO4
            case ESM::fourCC("XPLK"): // FO4
            case ESM::fourCC("XRFG"): // FO4
            case ESM::fourCC("XWPG"): // FO4
            case ESM::fourCC("XWPN"): // FO4
                // if (mFormId == 0x0007e90f) // XPRM XPOD
                // if (mBaseObj == 0x17) //XPRM XOCP occlusion plane data XMBO bound half extents
                reader.skipSubRecordData();
                break;
            default:
                throw std::runtime_error("ESM4::REFR::load - Unknown subrecord " + ESM::printName(subHdr.typeId));
        }
    }
    // if (mFormId == 0x0016B74B) // base is TACT vCasinoUltraLuxeRadio in cell ULCasino
    // std::cout << "REFR SCRO " << formIdToString(sid) << std::endl;
}

// void ESM4::Reference::save(ESM4::Writer& writer) const
//{
// }

void ESM4::Reference::blank() {}
