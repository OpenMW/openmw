/*
  Copyright (C) 2019-2021 cc9cii

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
#include "loadscpt.hpp"

#include <stdexcept>
#include <iostream> // FIXME: debugging only
#include <iomanip>

#include "reader.hpp"
//#include "writer.hpp"

void ESM4::Script::load(ESM4::Reader& reader)
{
    mFormId = reader.hdr().record.id;
    reader.adjustFormId(mFormId);
    mFlags  = reader.hdr().record.flags;

    static ScriptLocalVariableData localVar;

    while (reader.getSubRecordHeader())
    {
        const ESM4::SubRecordHeader& subHdr = reader.subRecordHeader();
        switch (subHdr.typeId)
        {
            case ESM4::SUB_EDID:
            {
                reader.getZString(mEditorId);
                break;
            }
            case ESM4::SUB_SCHR:
            {
    // For debugging only
#if 0
                unsigned char mDataBuf[256/*bufSize*/];
                reader.get(mDataBuf, subHdr.dataSize);

                std::ostringstream ss;
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
                reader.get(mScript.scriptHeader);
#endif
                break;
            }
            case ESM4::SUB_SCTX: reader.getString(mScript.scriptSource);
                //if (mEditorId == "CTrapLogs01SCRIPT")
                    //std::cout << mScript.scriptSource << std::endl;
                break;
            case ESM4::SUB_SCDA: // compiled script data
            {
    // For debugging only
#if 0
                if (subHdr.dataSize >= 4096)
                {
                    std::cout << "Skipping " << mEditorId << std::endl;
                    reader.skipSubRecordData();
                    break;
                }

                std::cout << mEditorId << std::endl;

                unsigned char mDataBuf[4096/*bufSize*/];
                reader.get(mDataBuf, subHdr.dataSize);

                std::ostringstream ss;
                for (unsigned int i = 0; i < subHdr.dataSize; ++i)
                {
                    //if (mDataBuf[i] > 64 && mDataBuf[i] < 91)
                        //ss << (char)(mDataBuf[i]) << " ";
                    //else
                        ss << std::setfill('0') << std::setw(2) << std::hex << (int)(mDataBuf[i]);
                    if ((i & 0x000f) == 0xf)
                        ss << "\n";
                    else if (i < 4096/*bufSize*/-1)
                        ss << " ";
                }
                std::cout << ss.str() << std::endl;
#else
                reader.skipSubRecordData();
#endif
                break;
            }
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
            default:
                //std::cout << "SCPT " << ESM::printName(subHdr.typeId) << " skipping..."
                          //<< subHdr.dataSize << std::endl;
                //reader.skipSubRecordData();
                //break;
                throw std::runtime_error("ESM4::SCPT::load - Unknown subrecord " + ESM::printName(subHdr.typeId));
        }
    }
}

//void ESM4::Script::save(ESM4::Writer& writer) const
//{
//}

//void ESM4::Script::blank()
//{
//}
