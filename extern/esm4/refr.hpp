/*
  Copyright (C) 2015-2016, 2018 cc9cii

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

#include "common.hpp" // Position

namespace ESM4
{
    class Reader;
    class Writer;

    struct TeleportDest
    {
        FormId   destDoor;
        Position destPos;
        std::uint32_t flags; // 0x01 no alarm (only in TES5)
    };

    // Unlike TES3, multiple cells can have the same exterior co-ordinates.
    // The cells need to be organised under world spaces.
    struct Reference
    {
        FormId mFormId;       // from the header
        std::uint32_t mFlags; // from the header, see enum type RecordFlag for details

        std::string mEditorId;
        std::string mFullName;
        FormId      mBaseObj;

        Position mPosition;
        float    mScale;      // default 1.f
        FormId   mOwner;
        FormId   mGlobal;
        std::uint32_t mFactionRank;

        bool mDisabled;
        EnableParent mEsp;    // TODO may need to check mFlags & 0x800 (initially disabled)

        std::uint32_t mCount; // only if > 1 (default 1)

        TeleportDest mDoor;

        Reference();
        virtual ~Reference();

        virtual void load(ESM4::Reader& reader);
        //virtual void save(ESM4::Writer& writer) const;

        void blank();
    };
}

#endif // ESM4_REFR_H
