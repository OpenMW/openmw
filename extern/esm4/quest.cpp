/*
  Copyright (C) 2019 cc9cii

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
#include "quest.hpp"

#include <stdexcept>
//#include <iostream> // FIXME: testing only

#include "common.hpp"
#include "reader.hpp"
#include "writer.hpp"

ESM4::Quest::Quest() : mFormId(0), mFlags(0), mScript(0), mCurrentLogEntry(nullptr), mCurrentStage(nullptr), mCurrentTarget(nullptr)
{
    mEditorId.clear();
    mQuestName.clear();
    mScriptResult.clear();
    mQuestData.mPriority = 0; mQuestData.mQuestFlags = 0;
    //mModel.clear();
}

ESM4::Quest::~Quest()
{
}

void ESM4::Quest::load(ESM4::Reader& reader)
{
    mFormId = reader.hdr().record.id;
    reader.adjustFormId(mFormId);
    mFlags  = reader.hdr().record.flags;

    while (reader.getSubRecordHeader())
    {
        const ESM4::SubRecordHeader& subHdr = reader.subRecordHeader();
        char mtemp[2048];
        switch (subHdr.typeId)
        {
        case ESM4::SUB_EDID: reader.getZString(mEditorId); break;
        case ESM4::SUB_FULL: reader.getZString(mQuestName); break;
        case ESM4::SUB_ICON: reader.getZString(mIconFileName); break;
        case ESM4::SUB_SCRI: reader.getFormId(mScript); break;
        case ESM4::SUB_DNAM: //reader.getFormId()
            reader.get((char*)mtemp,subHdr.dataSize);//4b
            break;
        case ESM4::SUB_ENAM:
            reader.get((char*)&mEvent,subHdr.dataSize);
            break;
        case ESM4::SUB_CTDA:
            //reader.get((char*)mtemp,subHdr.dataSize);//24b
            Condition cond;
            reader.get(cond.cdta, subHdr.dataSize);
            if(mCurrentTarget)
                mCurrentTarget->mConditions.push_back(cond);
            else if(mCurrentLogEntry)
                mCurrentLogEntry->conditions.push_back(cond);
            else mQuestConditions.push_back(cond);
            break;
        case ESM4::SUB_DATA:
            if(subHdr.dataSize==2)//tes4
            {
                reader.get(mQuestData.mQuestFlags);
                reader.get(mQuestData.mPriority);
            }
            else
            {//TODO
                reader.get((char*)mtemp,subHdr.dataSize);//2b?
            }
            break;
            case ESM4::SUB_INDX:
            {
                mCurrentTarget=0; mCurrentLogEntry=0;
                mStages.push_back(QuestStage());
                mCurrentStage = &mStages.back();
                reader.get(mCurrentStage->mIndex);
                //reader.get((char*)&mCurrentStage->mIndex, subHdr.dataSize);//2 tab
                break;
            }
            case ESM4::SUB_QSDT: //LOG Entry flags 0x01=Complete quest
            {
                mCurrentStage->mLogEntries.push_back(LogEntry());
                mCurrentLogEntry = &mCurrentStage->mLogEntries.back();
                reader.get(mCurrentLogEntry->mLogEntryFlag);//1b
                break;
            }
            case  ESM4::SUB_QSTA: //Quest Target  Target Alias Flags
            {
                mTargets.push_back(Target());
                mCurrentTarget = &mTargets.back();
                reader.getFormId(mCurrentTarget->mTargetAlias);
                reader.get(mCurrentTarget->mFlags);
                //reader.get((char*)mtemp,subHdr.dataSize);//8
                break;
            }
            case ESM4::SUB_SCDA: // Compiled Script Data
            {
                reader.get((char*)mtemp,subHdr.dataSize);//var
                break;
            }
            case ESM4::SUB_SCTX: // Result script source.
            {
            reader.getZString(mScriptResult);
                //assert(mCurrentLogEntry->mScript==nullptr);
                //mCurrentLogEntry->mScript=new char [subHdr.dataSize];
                //reader.get((char*)mtemp, subHdr.dataSize);//
                break;
            }
            case ESM4::SUB_SCRO: // global variable? reference
            {
               // assert(mGlobalVar==0);//todo tab or inject in children
                ESM4::FormId globalvar; reader.getFormId(globalvar); mGlobalVars.push_back(globalvar);
                break;
            }
            case ESM4::SUB_SCHR: //old script
            {
                reader.get((char*)mtemp,subHdr.dataSize);//var 20
                break;
            }
            case ESM4::SUB_CNAM: // 	Journal entry
            {
                reader.getZString(mCurrentLogEntry->mLogString);
                break;
            }
            /* TES>4
             * case ESM4::SUB_NAM0: // 	Journal entry
            {
                reader.get((char*)mtemp,subHdr.dataSize);//
                break;
            }

            case ESM4::SUB_QOBJ: // 	Quest objective
            {
                reader.get((char*)mtemp,subHdr.dataSize);//
                break;
            }
            case ESM4::SUB_FNAM: // 	Flags
            {
                reader.get((char*)&mtemp,subHdr.dataSize);//
                break;
            }
            case ESM4::SUB_NNAM: // 	Flags
            {
                reader.get((char*)mtemp,subHdr.dataSize);//
                break;
            }
            case ESM4::SUB_ANAM:reader.get((char*)mtemp,subHdr.dataSize);
            break;
            case ESM4::SUB_NEXT:reader.get((char*)mtemp,subHdr.dataSize);
            break;
            case ESM4::SUB_FLTR:
            reader.get((char*)mtemp,subHdr.dataSize);
            break;*/
            default:
                throw std::runtime_error("ESM4::QUST::load - Unknown subrecord " + ESM4::printName(subHdr.typeId));
        }
    }
}


void ESM4::Quest::save(ESM4::Writer& reader) const
{

}


