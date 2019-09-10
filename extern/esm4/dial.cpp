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
#include "dial.hpp"

#include <stdexcept>
//#include <iostream> // FIXME: testing only

#include "common.hpp"
#include "reader.hpp"
#include "vmad.hpp"
//#include "writer.hpp"

ESM4::Dialog::Dialog() : mFormId(0), mFlags(0)//, mData(0), mAdditionalPart(0)
{
    mEditorId.clear();
    //mModel.clear();
}

ESM4::Dialog::~Dialog()
{
}

void ESM4::Dialog::load(ESM4::Reader& reader)
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
            case ESM4::SUB_EDID: reader.getZString(mEditorId); break;
      /*  case ESM::SREC_NAME:
            mId = esm.getHString();
            hasName = true;
            break;*/
        case ESM4::SUB_FULL:
            reader.getZString(mPlayerDialog);
            break;
        case ESM4::SUB_QSTI:
        {
            ESM4::FormId quest;
            reader.getFormId(quest);
            mQuestid.push_back(quest);
            //reader.get((char*)&mQuestid,subHdr.dataSize);
        }
            break;
        case ESM4::SUB_DATA:
            char t;
            //reader.get((char*)&mDialogData,subHdr.dataSize);//1 byte most of the time (tes4)
            reader.get(t);
            mType = DialogType(t);
            break;


        case ESM4::SUB_PNAM:
            reader.get((char*)&mPriority,subHdr.dataSize);
            break;
        case ESM4::SUB_BNAM:
            reader.getFormId(mDLBR);
            break;
        case ESM4::SUB_QNAM:
            reader.getFormId(mQUST);
            //reader.get((char*)&mQUST,subHdr.dataSize);
            break;
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
           mDialogSubtype= MKTAG(fourcc[0],fourcc[1],fourcc[2],fourcc[3]);
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
                throw std::runtime_error("ESM4::Dialog::load - Unknown subrecord " + ESM4::printName(subHdr.typeId));
        }
    }
    //reader.getRecordData();

}


ESM4::DialogBranch::DialogBranch()
{
}

ESM4::DialogBranch::~DialogBranch()
{
}

void ESM4::Dialog::loadInfo(ESM4::Reader& reader)
{
    mVecInfoDial.push_back(Info());
    mVecInfoDial.back().load(reader);
}

void ESM4::DialogBranch::load(ESM4::Reader& reader)
{
    mFormId = reader.hdr().record.id;
    reader.adjustFormId(mFormId);
    mFlags  = reader.hdr().record.flags;

    while (reader.getSubRecordHeader())
    {
        const ESM4::SubRecordHeader& subHdr = reader.subRecordHeader();
        switch (subHdr.typeId)
        {
            case ESM4::SUB_EDID: reader.getZString(mEditorId); break;
        case ESM4::SUB_QNAM:
            reader.getFormId(mParentQuest);
            break;
        case ESM4::SUB_TNAM:
            reader.get(mTNAMUnknown);
            break;

        case ESM4::SUB_DNAM:
            reader.get(mDNAMflags);
            break;

        case ESM4::SUB_SNAM:
            reader.getFormId(mStartDialog);
            break;
            default:
                throw std::runtime_error("ESM4::DialogBranch::load - Unknown subrecord " + ESM4::printName(subHdr.typeId));
        }
    }
}
