/*
  Copyright (C) 2020-2021 cc9cii

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
#include "loadinfo.hpp"

#include <stdexcept>
#include <cstring>
#include <iostream> // FIXME: for debugging only

#include "reader.hpp"
//#include "writer.hpp"

void ESM4::DialogInfo::load(ESM4::Reader& reader)
{
    mFormId = reader.hdr().record.id;
    reader.adjustFormId(mFormId);
    mFlags  = reader.hdr().record.flags;

    mEditorId = formIdToString(mFormId); // FIXME: quick workaround to use existing code

    static ScriptLocalVariableData localVar;
    bool ignore = false;

    while (reader.getSubRecordHeader())
    {
        const ESM4::SubRecordHeader& subHdr = reader.subRecordHeader();
        switch (subHdr.typeId)
        {
            case ESM4::SUB_QSTI: reader.getFormId(mQuest); break; // FormId quest id
            case ESM4::SUB_SNDD: reader.getFormId(mSound); break; // FO3 (not used in FONV?)
            case ESM4::SUB_TRDT:
            {
                if (subHdr.dataSize == 16) // TES4
                    reader.get(&mResponseData, 16);
                else if (subHdr.dataSize == 20) // FO3
                    reader.get(&mResponseData, 20);
                else // FO3/FONV
                {
                    reader.get(mResponseData);
                    if (mResponseData.sound)
                        reader.adjustFormId(mResponseData.sound);
                }

                break;
            }
            case ESM4::SUB_NAM1: reader.getZString(mResponse); break; // response text
            case ESM4::SUB_NAM2: reader.getZString(mNotes); break; // actor notes
            case ESM4::SUB_NAM3: reader.getZString(mEdits); break; // not in TES4
            case ESM4::SUB_CTDA: // FIXME: how to detect if 1st/2nd param is a formid?
            {
                if (subHdr.dataSize == 24) // TES4
                    reader.get(&mTargetCondition, 24);
                else if (subHdr.dataSize == 20) // FO3
                    reader.get(&mTargetCondition, 20);
                else if (subHdr.dataSize == 28)
                {
                    reader.get(mTargetCondition); // FO3/FONV
                    if (mTargetCondition.reference)
                        reader.adjustFormId(mTargetCondition.reference);
                }
                else // TES5
                {
                    reader.get(&mTargetCondition, 20);
                    if (subHdr.dataSize == 36)
                        reader.getFormId(mParam3);
                    reader.get(mTargetCondition.runOn);
                    reader.get(mTargetCondition.reference);
                    if (mTargetCondition.reference)
                        reader.adjustFormId(mTargetCondition.reference);
                    reader.skipSubRecordData(4); // unknown
                }

                break;
            }
            case ESM4::SUB_SCHR:
            {
                if (!ignore)
                    reader.get(mScript.scriptHeader);
                else
                    reader.skipSubRecordData(); // TODO: does the second one ever used?

                break;
            }
            case ESM4::SUB_SCDA: reader.skipSubRecordData(); break; // compiled script data
            case ESM4::SUB_SCTX: reader.getString(mScript.scriptSource); break;
            case ESM4::SUB_SCRO: reader.getFormId(mScript.globReference); break;
            case ESM4::SUB_SLSD:
            {
                localVar.clear();
                reader.get(localVar.index);
                reader.get(localVar.unknown1);
                reader.get(localVar.unknown2);
                reader.get(localVar.unknown3);
                reader.get(localVar.type);
                reader.get(localVar.unknown4);
                // WARN: assumes SCVR will follow immediately

                break;
            }
            case ESM4::SUB_SCVR: // assumed always pair with SLSD
            {
                reader.getZString(localVar.variableName);

                mScript.localVarData.push_back(localVar);

                break;
            }
            case ESM4::SUB_SCRV:
            {
                std::uint32_t index;
                reader.get(index);

                mScript.localRefVarIndex.push_back(index);

                break;
            }
            case ESM4::SUB_NEXT: // FO3/FONV marker for next script header
            {
                ignore = true;

                break;
            }
            case ESM4::SUB_DATA: // always 3 for TES4 ?
            {
                if (subHdr.dataSize == 4) // FO3/FONV
                {
                    reader.get(mDialType);
                    reader.get(mNextSpeaker);
                    reader.get(mInfoFlags);
                }
                else
                    reader.skipSubRecordData(); // FIXME
                break;
            }
            case ESM4::SUB_NAME: // FormId add topic (not always present)
            case ESM4::SUB_CTDT: // older version of CTDA? 20 bytes
            case ESM4::SUB_SCHD: // 28 bytes
            case ESM4::SUB_TCLT: // FormId choice
            case ESM4::SUB_TCLF: // FormId
            case ESM4::SUB_PNAM: // TES4 DLC
            case ESM4::SUB_TPIC: // TES4 DLC
            case ESM4::SUB_ANAM: // FO3 speaker formid
            case ESM4::SUB_DNAM: // FO3 speech challenge
            case ESM4::SUB_KNAM: // FO3 formid
            case ESM4::SUB_LNAM: // FONV
            case ESM4::SUB_TCFU: // FONV
            case ESM4::SUB_TIFC: // TES5
            case ESM4::SUB_TWAT: // TES5
            case ESM4::SUB_CIS2: // TES5
            case ESM4::SUB_CNAM: // TES5
            case ESM4::SUB_ENAM: // TES5
            case ESM4::SUB_EDID: // TES5
            case ESM4::SUB_VMAD: // TES5
            case ESM4::SUB_BNAM: // TES5
            case ESM4::SUB_SNAM: // TES5
            case ESM4::SUB_ONAM: // TES5
            case ESM4::SUB_QNAM: // TES5 for mScript
            case ESM4::SUB_RNAM: // TES5
            {
                //std::cout << "INFO " << ESM::printName(subHdr.typeId) << " skipping..."
                        //<< subHdr.dataSize << std::endl;
                reader.skipSubRecordData();
                break;
            }
            default:
                std::cout << "INFO " << ESM::printName(subHdr.typeId) << " skipping..."
                        << subHdr.dataSize << std::endl;
                reader.skipSubRecordData();
                //throw std::runtime_error("ESM4::INFO::load - Unknown subrecord " + ESM::printName(subHdr.typeId));
        }
    }
}

//void ESM4::DialogInfo::save(ESM4::Writer& writer) const
//{
//}

//void ESM4::DialogInfo::blank()
//{
//}
