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
#ifndef ESM4_ARMA_H
#define ESM4_ARMA_H

#include <cstdint>
#include <string>
#include <vector>

#include <components/esm/defs.hpp>
#include <components/esm/formid.hpp>

#include "actor.hpp" // BodyTemplate

namespace ESM4
{
    class Reader;
    class Writer;

    struct ArmorAddon
    {
        ESM::FormId mId; // from the header
        std::uint32_t mFlags; // from the header, see enum type RecordFlag for details

        std::string mEditorId;

        std::string mModelMale;
        std::string mModelFemale;

        ESM::FormId mTextureMale;
        ESM::FormId mTextureFemale;

        ESM::FormId mRacePrimary;
        std::vector<ESM::FormId> mRaces; // TES5 only

        BodyTemplate mBodyTemplate; // TES5

        std::uint8_t mMalePriority = 0;
        std::uint8_t mFemalePriority = 0;

        // Flag 0x2 in mWeightSlider means that there are 2 world models for different weights: _0.nif and _1.nif
        std::uint8_t mWeightSliderMale = 0;
        std::uint8_t mWeightSliderFemale = 0;

        std::uint8_t mDetectionSoundValue = 0;
        float mWeaponAdjust = 0;

        void load(ESM4::Reader& reader);
        // void save(ESM4::Writer& writer) const;

        // void blank();
        static constexpr ESM::RecNameInts sRecordId = ESM::RecNameInts::REC_ARMA4;
    };
}

#endif // ESM4_ARMA_H
