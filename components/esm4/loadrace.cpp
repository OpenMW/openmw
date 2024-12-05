/*
  Copyright (C) 2016, 2018-2021 cc9cii

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
#include "loadrace.hpp"

#include <cstring>
#include <stdexcept>

#include "reader.hpp"
//#include "writer.hpp"

void ESM4::Race::load(ESM4::Reader& reader)
{
    mId = reader.getFormIdFromHeader();
    mFlags = reader.hdr().record.flags;

    std::uint32_t esmVer = reader.esmVersion();
    bool isTES4 = (esmVer == ESM::VER_080 || esmVer == ESM::VER_100) && !reader.hasFormVersion();
    bool isFONV = esmVer == ESM::VER_132 || esmVer == ESM::VER_133 || esmVer == ESM::VER_134;
    bool isFO3 = false;

    bool isMale = false;
    int curr_part = -1; // 0 = head, 1 = body, 2 = egt, 3 = hkx
    std::uint32_t currentIndex = 0xffffffff;

    while (reader.getSubRecordHeader())
    {
        const ESM4::SubRecordHeader& subHdr = reader.subRecordHeader();
        // std::cout << "RACE " << ESM::printName(subHdr.typeId) << std::endl;
        switch (subHdr.typeId)
        {
            case ESM::fourCC("EDID"):
            {
                reader.getZString(mEditorId);
                // TES4
                // Sheogorath  0x0005308E
                // GoldenSaint 0x0001208F
                // DarkSeducer 0x0001208E
                // VampireRace 0x00000019
                // Dremora     0x00038010
                // Argonian    0x00023FE9
                // Nord        0x000224FD
                // Breton      0x000224FC
                // WoodElf     0x000223C8
                // Khajiit     0x000223C7
                // DarkElf     0x000191C1
                // Orc         0x000191C0
                // HighElf     0x00019204
                // Redguard    0x00000D43
                // Imperial    0x00000907
                break;
            }
            case ESM::fourCC("FULL"):
                reader.getLocalizedString(mFullName);
                break;
            case ESM::fourCC("DESC"):
            {
                if (subHdr.dataSize == 1) // FO3?
                {
                    reader.skipSubRecordData();
                    break;
                }

                reader.getLocalizedString(mDesc);
                break;
            }
            case ESM::fourCC("SPLO"): // bonus spell formid (TES5 may have SPCT and multiple SPLO)
                reader.getFormId(mBonusSpells.emplace_back());
                break;
            case ESM::fourCC("DATA"): // ?? different length for TES5
            {
// DATA:size 128
// 0f 0f ff 00 ff 00 ff 00 ff 00 ff 00 ff 00 00 00
// 9a 99 99 3f 00 00 80 3f 00 00 80 3f 00 00 80 3f
// 48 89 10 00 00 00 40 41 00 00 00 00 00 00 48 43
// 00 00 48 43 00 00 80 3f 9a 99 19 3f 00 00 00 40
// 01 00 00 00 ff ff ff ff ff ff ff ff 00 00 00 00
// ff ff ff ff 00 00 00 00 00 00 20 41 00 00 a0 40
// 00 00 a0 40 00 00 80 42 ff ff ff ff 00 00 00 00
// 00 00 00 00 9a 99 99 3e 00 00 a0 40 02 00 00 00
#if 0
                unsigned char mDataBuf[256/*bufSize*/];
                reader.get(mDataBuf, subHdr.dataSize);

                std::ostringstream ss;
                ss << ESM::printName(subHdr.typeId) << ":size " << subHdr.dataSize << "\n";
                for (unsigned int i = 0; i < subHdr.dataSize; ++i)
                {
                    //if (mDataBuf[i] > 64 && mDataBuf[i] < 91)
                        //ss << (char)(mDataBuf[i]) << " ";
                    //else
                        ss << std::setfill('0') << std::setw(2) << std::hex << (int)(mDataBuf[i]);
                    if ((i & 0x000f) == 0xf)
                        ss << "\n";
                    else if (i < 256/*bufSize*/-1)
                        ss << " ";
                }
                std::cout << ss.str() << std::endl;
#else
                if (subHdr.dataSize == 36) // TES4/FO3/FONV
                {
                    if (!isTES4 && !isFONV && !mIsTES5)
                        isFO3 = true;

                    std::uint8_t skill;
                    std::uint8_t bonus;
                    for (unsigned int i = 0; i < 8; ++i)
                    {
                        reader.get(skill);
                        reader.get(bonus);
                        mSkillBonus[static_cast<SkillIndex>(skill)] = bonus;
                    }
                    reader.get(mHeightMale);
                    reader.get(mHeightFemale);
                    reader.get(mWeightMale);
                    reader.get(mWeightFemale);
                    reader.get(mRaceFlags);
                }
                else if (subHdr.dataSize == 128 || subHdr.dataSize == 164) // TES5
                {
                    mIsTES5 = true;

                    std::uint8_t skill;
                    std::uint8_t bonus;
                    for (unsigned int i = 0; i < 7; ++i)
                    {
                        reader.get(skill);
                        reader.get(bonus);
                        mSkillBonus[static_cast<SkillIndex>(skill)] = bonus;
                    }
                    std::uint16_t unknown;
                    reader.get(unknown);

                    reader.get(mHeightMale);
                    reader.get(mHeightFemale);
                    reader.get(mWeightMale);
                    reader.get(mWeightFemale);
                    reader.get(mRaceFlags);

                    // FIXME
                    float dummy;
                    reader.get(dummy); // starting health
                    reader.get(dummy); // starting magicka
                    reader.get(dummy); // starting stamina
                    reader.get(dummy); // base carry weight
                    reader.get(dummy); // base mass
                    reader.get(dummy); // accleration rate
                    reader.get(dummy); // decleration rate

                    uint32_t dummy2;
                    reader.get(dummy2); // size
                    reader.get(dummy2); // head biped object
                    reader.get(dummy2); // hair biped object
                    reader.get(dummy); // injured health % (0.f..1.f)
                    reader.get(dummy2); // shield biped object
                    reader.get(dummy); // health regen
                    reader.get(dummy); // magicka regen
                    reader.get(dummy); // stamina regen
                    reader.get(dummy); // unarmed damage
                    reader.get(dummy); // unarmed reach
                    reader.get(dummy2); // body biped object
                    reader.get(dummy); // aim angle tolerance
                    reader.get(dummy2); // unknown
                    reader.get(dummy); // angular accleration rate
                    reader.get(dummy); // angular tolerance
                    reader.get(dummy2); // flags

                    if (subHdr.dataSize == 164)
                    {
                        reader.get(dummy2); // unknown 1
                        reader.get(dummy2); // unknown 2
                        reader.get(dummy2); // unknown 3
                        reader.get(dummy2); // unknown 4
                        reader.get(dummy2); // unknown 5
                        reader.get(dummy2); // unknown 6
                        reader.get(dummy2); // unknown 7
                        reader.get(dummy2); // unknown 8
                        reader.get(dummy2); // unknown 9
                    }
                }
                else
                {
                    reader.skipSubRecordData();
                    // std::cout << "RACE " << ESM::printName(subHdr.typeId) << " skipping..." << subHdr.dataSize <<
                    // std::endl;
                }
#endif
                break;
            }
            case ESM::fourCC("DNAM"):
            {
                reader.getFormId(mDefaultHair[0]); // male
                reader.getFormId(mDefaultHair[1]); // female

                break;
            }
            case ESM::fourCC("CNAM"):
                //              CNAM       SNAM                     VNAM
                // Sheogorath   0x0  0000  98 2b  10011000 00101011
                // Golden Saint 0x3  0011  26 46  00100110 01000110
                // Dark Seducer 0xC  1100  df 55  11011111 01010101
                // Vampire Race 0x0  0000  77 44  01110111 10001000
                // Dremora      0x7  0111  bf 32  10111111 00110010
                // Argonian     0x0  0000  dc 3c  11011100 00111100
                // Nord         0x5  0101  b6 03  10110110 00000011
                // Breton       0x5  0101  48 1d  01001000 00011101 00000000 00000907 (Imperial)
                // Wood Elf     0xD  1101  2e 4a  00101110 01001010 00019204 00019204 (HighElf)
                // khajiit      0x5  0101  54 5b  01010100 01011011 00023FE9 00023FE9 (Argonian)
                // Dark Elf     0x0  0000  72 54  01110010 01010100 00019204 00019204 (HighElf)
                // Orc          0xC  1100  74 09  01110100 00001001 000224FD 000224FD (Nord)
                // High Elf     0xF  1111  e6 21  11100110 00100001
                // Redguard     0xD  1101  a9 61  10101001 01100001
                // Imperial     0xD  1101  8e 35  10001110 00110101
                {
                    reader.skipSubRecordData();
                    break;
                }
            case ESM::fourCC("PNAM"):
                reader.get(mFaceGenMainClamp);
                break; // 0x40A00000 = 5.f
            case ESM::fourCC("UNAM"):
                reader.get(mFaceGenFaceClamp);
                break; // 0x40400000 = 3.f
            case ESM::fourCC("ATTR"): // Only in TES4?
            {
                if (subHdr.dataSize == 2) // FO3?
                {
                    reader.skipSubRecordData();
                    break;
                }
                reader.get(mAttribMale.strength);
                reader.get(mAttribMale.intelligence);
                reader.get(mAttribMale.willpower);
                reader.get(mAttribMale.agility);
                reader.get(mAttribMale.speed);
                reader.get(mAttribMale.endurance);
                reader.get(mAttribMale.personality);
                reader.get(mAttribMale.luck);
                reader.get(mAttribFemale.strength);
                reader.get(mAttribFemale.intelligence);
                reader.get(mAttribFemale.willpower);
                reader.get(mAttribFemale.agility);
                reader.get(mAttribFemale.speed);
                reader.get(mAttribFemale.endurance);
                reader.get(mAttribFemale.personality);
                reader.get(mAttribFemale.luck);

                break;
            }
            //        [0..9]-> ICON
            // NAM0 -> INDX -> MODL --+
            //          ^   -> MODB   |
            //          |             |
            //          +-------------+
            //
            case ESM::fourCC("NAM0"): // start marker head data /* 1 */
            {
                curr_part = 0; // head part

                if (isFO3 || isFONV)
                {
                    mHeadParts.resize(8);
                    mHeadPartsFemale.resize(8);
                }
                else if (isTES4)
                    mHeadParts.resize(9); // assumed based on Construction Set
                else // Optimized for TES5
                {
                    mHeadPartIdsMale.resize(5);
                    mHeadPartIdsFemale.resize(5);
                }

                currentIndex = 0xffffffff;
                break;
            }
            case ESM::fourCC("INDX"):
            {
                reader.get(currentIndex);
                // FIXME: below check is rather useless
                // if (headpart)
                //{
                //    if (currentIndex > 8)
                //        throw std::runtime_error("ESM4::RACE::load - too many head part " + currentIndex);
                //}
                // else // bodypart
                //{
                //    if (currentIndex > 4)
                //        throw std::runtime_error("ESM4::RACE::load - too many body part " + currentIndex);
                //}

                break;
            }
            case ESM::fourCC("MODL"):
            {
                if (currentIndex == 0xffffffff)
                {
                    reader.skipSubRecordData();
                }
                else if (curr_part == 0) // head part
                {
                    if (isMale || isTES4)
                        reader.getZString(mHeadParts[currentIndex].mesh);
                    else
                        reader.getZString(mHeadPartsFemale[currentIndex].mesh); // TODO: check TES4

                    // TES5 keeps head part formid in mHeadPartIdsMale and mHeadPartIdsFemale
                }
                else if (curr_part == 1) // body part
                {
                    if (isMale)
                        reader.getZString(mBodyPartsMale[currentIndex].mesh);
                    else
                        reader.getZString(mBodyPartsFemale[currentIndex].mesh);

                    // TES5 seems to have no body parts at all, instead keep EGT models
                }
                // else if (curr_part == 2) // egt
                // {
                //     // std::cout << mEditorId << " egt " << currentIndex << std::endl; // FIXME
                //     reader.skipSubRecordData(); // FIXME TES5 egt
                // }
                else
                {
                    // std::cout << mEditorId << " hkx " << currentIndex << std::endl; // FIXME
                    reader.skipSubRecordData(); // FIXME TES5 hkx
                }

                break;
            }
            case ESM::fourCC("MODB"):
                reader.skipSubRecordData();
                break; // always 0x0000?
            case ESM::fourCC("ICON"):
            {
                if (currentIndex == 0xffffffff)
                {
                    reader.skipSubRecordData();
                }
                else if (curr_part == 0) // head part
                {
                    if (isMale || isTES4)
                        reader.getZString(mHeadParts[currentIndex].texture);
                    else
                        reader.getZString(mHeadPartsFemale[currentIndex].texture); // TODO: check TES4
                }
                else if (curr_part == 1) // body part
                {
                    if (isMale)
                        reader.getZString(mBodyPartsMale[currentIndex].texture);
                    else
                        reader.getZString(mBodyPartsFemale[currentIndex].texture);
                }
                else
                    reader.skipSubRecordData(); // FIXME TES5

                break;
            }
            //
            case ESM::fourCC("NAM1"): // start marker body data /* 4 */
            {

                if (isFO3 || isFONV)
                {
                    curr_part = 1; // body part

                    mBodyPartsMale.resize(4);
                    mBodyPartsFemale.resize(4);
                }
                else if (isTES4)
                {
                    curr_part = 1; // body part

                    mBodyPartsMale.resize(5); // 0 = upper body, 1 = legs, 2 = hands, 3 = feet, 4 = tail
                    mBodyPartsFemale.resize(5); // 0 = upper body, 1 = legs, 2 = hands, 3 = feet, 4 = tail
                }
                else // TES5
                    curr_part = 2; // for TES5 NAM1 indicates the start of EGT model

                if (isTES4)
                    currentIndex = 4; // FIXME: argonian tail mesh without preceeding INDX
                else
                    currentIndex = 0xffffffff;

                break;
            }
            case ESM::fourCC("MNAM"):
                isMale = true;
                break; /* 2, 5, 7 */
            case ESM::fourCC("FNAM"):
                isMale = false;
                break; /* 3, 6, 8 */
            //
            case ESM::fourCC("HNAM"):
            {
                // FIXME: this is a texture name in FO4
                if (subHdr.dataSize % sizeof(ESM::FormId32) != 0)
                    reader.skipSubRecordData();
                else
                {
                    std::size_t numHairChoices = subHdr.dataSize / sizeof(ESM::FormId32);
                    mHairChoices.resize(numHairChoices);
                    for (unsigned int i = 0; i < numHairChoices; ++i)
                        reader.getFormId(mHairChoices.at(i));
                }

                break;
            }
            case ESM::fourCC("ENAM"):
            {
                std::size_t numEyeChoices = subHdr.dataSize / sizeof(ESM::FormId32);
                mEyeChoices.resize(numEyeChoices);
                for (unsigned int i = 0; i < numEyeChoices; ++i)
                    reader.getFormId(mEyeChoices.at(i));

                break;
            }
            case ESM::fourCC("FGGS"):
            {
                if (isMale || isTES4)
                {
                    mSymShapeModeCoefficients.resize(50);
                    for (std::size_t i = 0; i < 50; ++i)
                        reader.get(mSymShapeModeCoefficients.at(i));
                }
                else
                {
                    mSymShapeModeCoeffFemale.resize(50);
                    for (std::size_t i = 0; i < 50; ++i)
                        reader.get(mSymShapeModeCoeffFemale.at(i));
                }

                break;
            }
            case ESM::fourCC("FGGA"):
            {
                if (isMale || isTES4)
                {
                    mAsymShapeModeCoefficients.resize(30);
                    for (std::size_t i = 0; i < 30; ++i)
                        reader.get(mAsymShapeModeCoefficients.at(i));
                }
                else
                {
                    mAsymShapeModeCoeffFemale.resize(30);
                    for (std::size_t i = 0; i < 30; ++i)
                        reader.get(mAsymShapeModeCoeffFemale.at(i));
                }

                break;
            }
            case ESM::fourCC("FGTS"):
            {
                if (isMale || isTES4)
                {
                    mSymTextureModeCoefficients.resize(50);
                    for (std::size_t i = 0; i < 50; ++i)
                        reader.get(mSymTextureModeCoefficients.at(i));
                }
                else
                {
                    mSymTextureModeCoeffFemale.resize(50);
                    for (std::size_t i = 0; i < 50; ++i)
                        reader.get(mSymTextureModeCoeffFemale.at(i));
                }

                break;
            }
            //
            case ESM::fourCC("SNAM"): // skipping...2 // only in TES4?
            {
                reader.skipSubRecordData();
                break;
            }
            case ESM::fourCC("XNAM"):
            {
                ESM::FormId race;
                std::int32_t adjustment;
                reader.getFormId(race);
                reader.get(adjustment);
                mDisposition[race] = adjustment;

                break;
            }
            case ESM::fourCC("VNAM"):
            {
                if (subHdr.dataSize == 8) // TES4
                {
                    reader.getFormId(mVNAM[0]); // For TES4 seems to be 2 race formids
                    reader.getFormId(mVNAM[1]);
                }
                else if (subHdr.dataSize == 4) // TES5
                {
                    // equipment type flags meant to be uint32 ???  GLOB reference? shows up in
                    // SCRO in sript records and CTDA in INFO records
                    uint32_t dummy;
                    reader.get(dummy);
                }
                else
                {
                    reader.skipSubRecordData();
                    // std::cout << "RACE " << ESM::printName(subHdr.typeId) << " skipping..." << subHdr.dataSize <<
                    // std::endl;
                }

                break;
            }
            //
            case ESM::fourCC("ANAM"): // TES5
            {
                if (isMale)
                    reader.getZString(mModelMale);
                else
                    reader.getZString(mModelFemale);
                break;
            }
            case ESM::fourCC("KSIZ"):
                reader.get(mNumKeywords);
                break;
            case ESM::fourCC("KWDA"):
            {
                ESM::FormId formid;
                for (unsigned int i = 0; i < mNumKeywords; ++i)
                    reader.getFormId(formid);
                break;
            }
            //
            case ESM::fourCC("WNAM"): // ARMO FormId
            {
                reader.getFormId(mSkin);
                // std::cout << mEditorId << " skin " << formIdToString(mSkin) << std::endl; // FIXME
                break;
            }
            case ESM::fourCC("BODT"): // body template
            {
                reader.get(mBodyTemplate.bodyPart);
                reader.get(mBodyTemplate.flags);
                reader.get(mBodyTemplate.unknown1); // probably padding
                reader.get(mBodyTemplate.unknown2); // probably padding
                reader.get(mBodyTemplate.unknown3); // probably padding
                reader.get(mBodyTemplate.type);

                break;
            }
            case ESM::fourCC("BOD2"):
            {
                if (subHdr.dataSize == 8 || subHdr.dataSize == 4) // TES5, FO4
                {
                    reader.get(mBodyTemplate.bodyPart);
                    mBodyTemplate.flags = 0;
                    mBodyTemplate.unknown1 = 0; // probably padding
                    mBodyTemplate.unknown2 = 0; // probably padding
                    mBodyTemplate.unknown3 = 0; // probably padding
                    mBodyTemplate.type = 0;
                    if (subHdr.dataSize == 8)
                        reader.get(mBodyTemplate.type);
                }
                else
                {
                    reader.skipSubRecordData();
                }

                break;
            }
            case ESM::fourCC("HEAD"): // TES5
            {
                ESM::FormId formId;
                reader.getFormId(formId);

                if (currentIndex != 0xffffffff)
                {
                    // FIXME: no order? head, mouth, eyes, brow, hair
                    if (isMale)
                    {
                        if (currentIndex >= mHeadPartIdsMale.size())
                            mHeadPartIdsMale.resize(currentIndex + 1);
                        mHeadPartIdsMale[currentIndex] = formId;
                    }
                    else
                    {
                        if (currentIndex >= mHeadPartIdsFemale.size())
                            mHeadPartIdsFemale.resize(currentIndex + 1);
                        mHeadPartIdsFemale[currentIndex] = formId;
                    }
                }

                // std::cout << mEditorId << (isMale ? " male head " : " female head ")
                // << formIdToString(formId) << " " << currentIndex << std::endl; // FIXME

                break;
            }
            case ESM::fourCC("NAM3"): // start of hkx model
            {
                curr_part = 3; // for TES5 NAM3 indicates the start of hkx model

                break;
            }
            // Not sure for what this is used - maybe to indicate which slots are in use? e.g.:
            //
            // ManakinRace HEAD
            // ManakinRace Hair
            // ManakinRace BODY
            // ManakinRace Hands
            // ManakinRace Forearms
            // ManakinRace Amulet
            // ManakinRace Ring
            // ManakinRace Feet
            // ManakinRace Calves
            // ManakinRace SHIELD
            // ManakinRace
            // ManakinRace LongHair
            // ManakinRace Circlet
            // ManakinRace
            // ManakinRace
            // ManakinRace
            // ManakinRace
            // ManakinRace
            // ManakinRace
            // ManakinRace
            // ManakinRace DecapitateHead
            // ManakinRace Decapitate
            // ManakinRace
            // ManakinRace
            // ManakinRace
            // ManakinRace
            // ManakinRace
            // ManakinRace
            // ManakinRace
            // ManakinRace
            // ManakinRace
            // ManakinRace FX0
            case ESM::fourCC("NAME"): // TES5 biped object names (x32)
            {
                std::string name;
                reader.getZString(name);
                // std::cout << mEditorId << " " << name << std::endl;

                break;
            }
            case ESM::fourCC("MTNM"): // movement type
            case ESM::fourCC("ATKD"): // attack data
            case ESM::fourCC("ATKE"): // attach event
            case ESM::fourCC("GNAM"): // body part data
            case ESM::fourCC("NAM4"): // material type
            case ESM::fourCC("NAM5"): // unarmed impact?
            case ESM::fourCC("LNAM"): // close loot sound
            case ESM::fourCC("QNAM"): // equipment slot formid
            case ESM::fourCC("HCLF"): // default hair colour
            case ESM::fourCC("UNES"): // unarmed equipment slot formid
            case ESM::fourCC("TINC"):
            case ESM::fourCC("TIND"):
            case ESM::fourCC("TINI"):
            case ESM::fourCC("TINL"):
            case ESM::fourCC("TINP"):
            case ESM::fourCC("TINT"):
            case ESM::fourCC("TINV"):
            case ESM::fourCC("TIRS"):
            case ESM::fourCC("PHWT"):
            case ESM::fourCC("AHCF"):
            case ESM::fourCC("AHCM"):
            case ESM::fourCC("MPAI"):
            case ESM::fourCC("MPAV"):
            case ESM::fourCC("DFTF"):
            case ESM::fourCC("DFTM"):
            case ESM::fourCC("FLMV"):
            case ESM::fourCC("FTSF"):
            case ESM::fourCC("FTSM"):
            case ESM::fourCC("MTYP"):
            case ESM::fourCC("NAM7"):
            case ESM::fourCC("NAM8"):
            case ESM::fourCC("PHTN"):
            case ESM::fourCC("RNAM"):
            case ESM::fourCC("RNMV"):
            case ESM::fourCC("RPRF"):
            case ESM::fourCC("RPRM"):
            case ESM::fourCC("SNMV"):
            case ESM::fourCC("SPCT"):
            case ESM::fourCC("SPED"):
            case ESM::fourCC("SWMV"):
            case ESM::fourCC("WKMV"):
            case ESM::fourCC("SPMV"):
            case ESM::fourCC("ATKR"):
            case ESM::fourCC("CTDA"):
            case ESM::fourCC("CIS1"):
            case ESM::fourCC("CIS2"):
            case ESM::fourCC("MODT"): // Model data
            case ESM::fourCC("MODC"):
            case ESM::fourCC("MODS"):
            case ESM::fourCC("MODF"): // Model data end
            //
            case ESM::fourCC("YNAM"): // FO3
            case ESM::fourCC("NAM2"): // FO3
            case ESM::fourCC("VTCK"): // FO3
            case ESM::fourCC("MODD"): // FO3
            case ESM::fourCC("ONAM"): // FO3
            case ESM::fourCC("APPR"): // FO4
            case ESM::fourCC("ATKS"): // FO4
            case ESM::fourCC("ATKT"): // FO4
            case ESM::fourCC("ATKW"): // FO4
            case ESM::fourCC("BMMP"): // FO4
            case ESM::fourCC("BSMB"): // FO4
            case ESM::fourCC("BSMP"): // FO4
            case ESM::fourCC("BSMS"): // FO4

            case ESM::fourCC("FMRI"): // FO4
            case ESM::fourCC("FMRN"): // FO4
            case ESM::fourCC("HLTX"): // FO4
            case ESM::fourCC("MLSI"): // FO4
            case ESM::fourCC("MPGN"): // FO4
            case ESM::fourCC("MPGS"): // FO4
            case ESM::fourCC("MPPC"): // FO4
            case ESM::fourCC("MPPF"): // FO4
            case ESM::fourCC("MPPI"): // FO4
            case ESM::fourCC("MPPK"): // FO4
            case ESM::fourCC("MPPM"): // FO4
            case ESM::fourCC("MPPN"): // FO4
            case ESM::fourCC("MPPT"): // FO4
            case ESM::fourCC("MSID"): // FO4
            case ESM::fourCC("MSM0"): // FO4
            case ESM::fourCC("MSM1"): // FO4
            case ESM::fourCC("NNAM"): // FO4
            case ESM::fourCC("NTOP"): // FO4
            case ESM::fourCC("PRPS"): // FO4
            case ESM::fourCC("PTOP"): // FO4
            case ESM::fourCC("QSTI"): // FO4
            case ESM::fourCC("RBPC"): // FO4
            case ESM::fourCC("SADD"): // FO4
            case ESM::fourCC("SAKD"): // FO4
            case ESM::fourCC("SAPT"): // FO4
            case ESM::fourCC("SGNM"): // FO4
            case ESM::fourCC("SRAC"): // FO4
            case ESM::fourCC("SRAF"): // FO4
            case ESM::fourCC("STCP"): // FO4
            case ESM::fourCC("STKD"): // FO4
            case ESM::fourCC("TETI"): // FO4
            case ESM::fourCC("TTEB"): // FO4
            case ESM::fourCC("TTEC"): // FO4
            case ESM::fourCC("TTED"): // FO4
            case ESM::fourCC("TTEF"): // FO4
            case ESM::fourCC("TTET"): // FO4
            case ESM::fourCC("TTGE"): // FO4
            case ESM::fourCC("TTGP"): // FO4
            case ESM::fourCC("UNWP"): // FO4
            case ESM::fourCC("WMAP"): // FO4
            case ESM::fourCC("ZNAM"): // FO4
                reader.skipSubRecordData();
                break;
            default:
                throw std::runtime_error("ESM4::RACE::load - Unknown subrecord " + ESM::printName(subHdr.typeId));
        }
    }
}

// void ESM4::Race::save(ESM4::Writer& writer) const
//{
// }

// void ESM4::Race::blank()
//{
// }
