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
#include "info.hpp"

#include <stdexcept>
//#include <iostream> // FIXME: testing only

#include "common.hpp"
#include "reader.hpp"
#include "vmad.hpp"
//#include "writer.hpp"

ESM4::Info::Info() : mFormId(0), mFlags(0), mCurrentChoice(nullptr), mCurrentResponse(nullptr)//, mData(0), mAdditionalPart(0)
{
    mEditorId.clear();
    mFullName.clear();
    //mModel.clear();
}

ESM4::Info::~Info()
{
}

void ESM4::Info::load(ESM4::Reader& reader)
{
    mFormId = reader.hdr().record.id;
    reader.adjustFormId(mFormId);
    mFlags  = reader.hdr().record.flags;

 //reader.hdr().record.version

    while (reader.getSubRecordHeader())
    {
        const ESM4::SubRecordHeader& subHdr = reader.subRecordHeader();

        switch (subHdr.typeId)
        {
        case ESM4::SUB_DATA: assert(subHdr.dataSize==3);reader.get((char*)&mDialogData,3); break;
        case ESM4::SUB_QSTI: reader.getFormId(mQuestId); break;
        case ESM4::SUB_PNAM: reader.getFormId(mPreviousInfoId);
            break;

        case ESM4::SUB_TRDT:
            //new reponse
            assert(sizeof(Emotion)==subHdr.dataSize);
            mResponse.push_back(Response());
            mCurrentResponse = &mResponse.back();
            reader.get(mCurrentResponse->mEmotion);
            break;
        //case ESM4::SUB_NAM0:    reader.getZString(mCurrentResponse->mResponseText);           break;
        case ESM4::SUB_NAM1:    reader.getZString(mCurrentResponse->mResponseText);           break;
        case ESM4::SUB_NAM2:    reader.getZString(mCurrentResponse->mActorNotes);           break;
        case ESM4::SUB_CTDA:
            assert(sizeof(Condition)==subHdr.dataSize);

            Condition cond; reader.get(cond); mConditions.push_back(cond);
            break;
        case ESM4::SUB_TCLT:
            mChoices.push_back(DialogChoice());
            mCurrentChoice = &mChoices.back();
        reader.getFormId(mCurrentChoice->mChoiceID);
            break;
        case ESM4::SUB_NAME:
         {
            ESM4::FormId topic;
            reader.getFormId(topic);
            mAddTopic.push_back(topic);
            break;}
        case ESM4::SUB_SCTX: //script result string
            reader.getZString(mResultScript);break;
        case ESM4::SUB_SCDA: //cmpioled script
        case ESM4::SUB_SCHR: //old script
        case ESM4::SUB_SCRO: //global var formid
        {
            char mtemp[2048];
            reader.get((char*)mtemp,subHdr.dataSize);//
            break;
        }
        case MKTAG('C','T','D','T')://ESM4::SUB_CTDT: //CTDA less 4byte unused
        {
            char mtemp[2048];
            reader.get((char*)mtemp,subHdr.dataSize);//20
            break;
        }
        case MKTAG('T','C','L','F')://ESM4::SUB_TCLF:
        {
            char mtemp[2048];
            reader.get((char*)mtemp,subHdr.dataSize);//
            break;
        }
         case MKTAG('S','C','H','D')://    ESM4::SUB_SCHD
        {
            char mtemp[2048];
            reader.get((char*)mtemp,subHdr.dataSize);//28
            break;
        }
        /*case ESM4::SUB_BNAM:
        {
            char mtemp[2048];
            reader.get((char*)mtemp,subHdr.dataSize);//
            break;
        }
        case ESM4::SUB_QNAM:
        {
            char mtemp[2048];
            reader.get((char*)mtemp,subHdr.dataSize);//
            break;
        }
        case ESM4::SUB_TIFC:
            reader.get(mInfoCount);

            struct PerFrag{
                std::uint8_t unknow0;std::string scriptname,fragmentname;
            };
            struct INFOFragment
            {
                std::uint8_t unknow0, flags;
                std::string filename;
                std::vector<PerFrag> mFragmentInfos;

            };
            for(int i=0;i<mInfoCount;++i)
            {
                std::string temp;
                reader.getZString(temp);

                VMADRecord<INFOFragment> vmad;

            }
            break;
        case ESM4::SUB_SNAM:
            unsigned char fourcc[4];
            reader.get((char*)fourcc,subHdr.dataSize);
           mINFOogSubtype= MKTAG(fourcc[0],fourcc[1],fourcc[2],fourcc[3]);
            break;


            /*
            case ESM4::SUB_FULL:
            {
                if (reader.hasLocalizedStrings())
                    reader.getLocalizedString(mFullName);
                else if (!reader.getZString(mFullName))
                    throw std::runtime_error ("HDPT FULL data read error");

                break;
            }
            case ESM4::SUB_DATA: reader.get(mData); break;
            case ESM4::SUB_MODL: reader.getZString(mModel); break;
            case ESM4::SUB_HNAM: reader.get(mAdditionalPart); break;
            case ESM4::SUB_PNAM:
            case ESM4::SUB_MODS:
            case ESM4::SUB_MODT:
            case ESM4::SUB_NAM0:
            case ESM4::SUB_NAM1:
            case ESM4::SUB_RNAM:
            case ESM4::SUB_TNAM:
            {
                //std::cout << "HDPT " << ESM4::printName(subHdr.typeId) << " skipping..." << std::endl;
                reader.skipSubRecordData();
                break;
            }*/
            default:
                throw std::runtime_error("ESM4::Info::load - Unknown subrecord " + ESM4::printName(subHdr.typeId));
        }
    }
    //reader.getRecordData();

}

