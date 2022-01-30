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
#ifndef ESM4_BPTD_H
#define ESM4_BPTD_H

#include <vector>

#include "formid.hpp"

namespace ESM4
{
    class Reader;
    class Writer;

    struct BodyPartData
    {
#pragma pack(push, 1)
        struct BPND
        {
            float damageMult;

            // Severable
            // IK Data
            // IK Data - Biped Data
            // Explodable
            // IK Data - Is Head
            // IK Data - Headtracking
            // To Hit Chance - Absolute
            std::uint8_t flags;

            // Torso
            // Head
            // Eye
            // LookAt
            // Fly Grab
            // Saddle
            std::uint8_t partType;

            std::uint8_t healthPercent;
            std::int8_t  actorValue; //(Actor Values)
            std::uint8_t toHitChance;

            std::uint8_t explExplosionChance; // %
            std::uint16_t explDebrisCount;
            FormId explDebris;
            FormId explExplosion;
            float trackingMaxAngle;
            float explDebrisScale;

            std::int32_t sevDebrisCount;
            FormId sevDebris;
            FormId sevExplosion;
            float sevDebrisScale;

            //Struct - Gore Effects Positioning
            float transX;
            float transY;
            float transZ;
            float rotX;
            float rotY;
            float rotZ;

            FormId sevImpactDataSet;
            FormId explImpactDataSet;
            uint8_t sevDecalCount;
            uint8_t explDecalCount;
            uint16_t Unknown;
            float limbReplacementScale;
        };
#pragma pack(pop)

        struct BodyPart
        {
            std::string mPartName;
            std::string mNodeName;
            std::string mVATSTarget;
            std::string mIKStartNode;
            BPND mData;
            std::string mLimbReplacementModel;
            std::string mGoreEffectsTarget;

            void clear();
        };

        FormId mFormId;       // from the header
        std::uint32_t mFlags; // from the header, see enum type RecordFlag for details

        std::string mEditorId;
        std::string mFullName;
        std::string mModel;

        std::vector<BodyPart> mBodyParts;

        BodyPartData();
        virtual ~BodyPartData();

        virtual void load(ESM4::Reader& reader);
        //virtual void save(ESM4::Writer& writer) const;

        //void blank();
    };
}

#endif // ESM4_BPTD_H
