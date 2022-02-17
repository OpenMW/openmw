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
#ifndef ESM4_REFR_H
#define ESM4_REFR_H

#include <cstdint>

#include "reference.hpp" // FormId, Placement, EnableParent

namespace ESM4
{
    class Reader;
    class Writer;

    enum MapMarkerType
    {
        Map_None          = 0x00, // ?
        Map_Camp          = 0x01,
        Map_Cave          = 0x02,
        Map_City          = 0x03,
        Map_ElvenRuin     = 0x04,
        Map_FortRuin      = 0x05,
        Map_Mine          = 0x06,
        Map_Landmark      = 0x07,
        Map_Tavern        = 0x08,
        Map_Settlement    = 0x09,
        Map_DaedricShrine = 0x0A,
        Map_OblivionGate  = 0x0B,
        Map_Unknown       = 0x0C // ? (door icon)
    };

    struct TeleportDest
    {
        FormId   destDoor;
        Placement destPos;
        std::uint32_t flags; // 0x01 no alarm (only in TES5)
    };

    struct RadioStationData
    {
        float rangeRadius;
        // 0 radius, 1 everywhere, 2 worldspace and linked int, 3 linked int, 4 current cell only
        std::uint32_t broadcastRange;
        float staticPercentage;
        FormId posReference; // only used if broadcastRange == 0
    };

    struct Reference
    {
        FormId mParent;       // cell FormId (currently persistent refs only), from the loading sequence
                              // NOTE: for exterior cells it will be the dummy cell FormId

        FormId mFormId;       // from the header
        std::uint32_t mFlags; // from the header, see enum type RecordFlag for details

        std::string mEditorId;
        std::string mFullName;
        FormId      mBaseObj;

        Placement mPlacement;
        float    mScale;      // default 1.f
        FormId   mOwner;
        FormId   mGlobal;
        std::int32_t mFactionRank;

        bool mInitiallyDisabled; // TODO may need to check mFlags & 0x800 (initially disabled)
        bool mIsMapMarker;
        std::uint16_t mMapMarker;

        EnableParent mEsp;

        std::uint32_t mCount; // only if > 1 (default 1)

        FormId mAudioLocation;

        RadioStationData mRadio;

        TeleportDest mDoor;
        bool mIsLocked;
        std::int8_t mLockLevel;
        FormId mKey;

        FormId mTargetRef;

        Reference();
        virtual ~Reference();

        virtual void load(ESM4::Reader& reader);
        //virtual void save(ESM4::Writer& writer) const;

        void blank();
    };
}

#endif // ESM4_REFR_H
