#include "loadinfo.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

namespace ESM
{
    unsigned int DialInfo::sRecordId = REC_INFO;

    void DialInfo::load(ESMReader &esm, bool &isDeleted)
    {
        mId = esm.getHNString("INAM");

        isDeleted = false;

        mQuestStatus = QS_None;
        mFactionLess = false;

        mPrev = esm.getHNString("PNAM");
        mNext = esm.getHNString("NNAM");

        while (esm.hasMoreSubs())
        {
            esm.getSubName();
            switch (esm.retSubName().intval)
            {
                case ESM::FourCC<'D','A','T','A'>::value:
                    esm.getHT(mData, 12);
                    break;
                case ESM::FourCC<'O','N','A','M'>::value:
                    mActor = esm.getHString();
                    break;
                case ESM::FourCC<'R','N','A','M'>::value:
                    mRace = esm.getHString();
                    break;
                case ESM::FourCC<'C','N','A','M'>::value:
                    mClass = esm.getHString();
                    break;
                case ESM::FourCC<'F','N','A','M'>::value:
                {
                    mFaction = esm.getHString();
                    if (mFaction == "FFFF")
                    {
                        mFactionLess = true;
                    }
                    break;
                }
                case ESM::FourCC<'A','N','A','M'>::value:
                    mCell = esm.getHString();
                    break;
                case ESM::FourCC<'D','N','A','M'>::value:
                    mPcFaction = esm.getHString();
                    break;
                case ESM::FourCC<'S','N','A','M'>::value:
                    mSound = esm.getHString();
                    break;
                case ESM::SREC_NAME:
                    mResponse = esm.getHString();
                    break;
                case ESM::FourCC<'S','C','V','R'>::value:
                {
                    SelectStruct ss;
                    ss.mSelectRule = esm.getHString();
                    ss.mValue.read(esm, Variant::Format_Info);
                    mSelects.push_back(ss);
                    break;
                }
                case ESM::FourCC<'B','N','A','M'>::value:
                    mResultScript = esm.getHString();
                    break;
                case ESM::FourCC<'Q','S','T','N'>::value:
                    mQuestStatus = QS_Name;
                    esm.skipRecord();
                    break;
                case ESM::FourCC<'Q','S','T','F'>::value:
                    mQuestStatus = QS_Finished;
                    esm.skipRecord();
                    break;
                case ESM::FourCC<'Q','S','T','R'>::value:
                    mQuestStatus = QS_Restart;
                    esm.skipRecord();
                    break;
                case ESM::SREC_DELE:
                    esm.skipHSub();
                    isDeleted = true;
                    break;
                default:
                    esm.fail("Unknown subrecord");
                    break;
            }
        }
    }

    void DialInfo::save(ESMWriter &esm, bool isDeleted) const
    {
        esm.writeHNCString("INAM", mId);
        esm.writeHNCString("PNAM", mPrev);
        esm.writeHNCString("NNAM", mNext);

        if (isDeleted)
        {
            esm.writeHNString("DELE", "", 3);
            return;
        }

        esm.writeHNT("DATA", mData, 12);
        esm.writeHNOCString("ONAM", mActor);
        esm.writeHNOCString("RNAM", mRace);
        esm.writeHNOCString("CNAM", mClass);
        esm.writeHNOCString("FNAM", mFaction);
        esm.writeHNOCString("ANAM", mCell);
        esm.writeHNOCString("DNAM", mPcFaction);
        esm.writeHNOCString("SNAM", mSound);
        esm.writeHNOString("NAME", mResponse);

        for (std::vector<SelectStruct>::const_iterator it = mSelects.begin(); it != mSelects.end(); ++it)
        {
            esm.writeHNString("SCVR", it->mSelectRule);
            it->mValue.write (esm, Variant::Format_Info);
        }

        esm.writeHNOString("BNAM", mResultScript);

        switch(mQuestStatus)
        {
        case QS_Name: esm.writeHNT("QSTN",'\1'); break;
        case QS_Finished: esm.writeHNT("QSTF", '\1'); break;
        case QS_Restart: esm.writeHNT("QSTR", '\1'); break;
        default: break;
        }
    }

    void DialInfo::blank()
    {
        mData.mUnknown1 = 0;
        mData.mDisposition = 0;
        mData.mRank = 0;
        mData.mGender = 0;
        mData.mPCrank = 0;
        mData.mUnknown2 = 0;

        mSelects.clear();
        mPrev.clear();
        mNext.clear();
        mActor.clear();
        mRace.clear();
        mClass.clear();
        mFaction.clear();
        mPcFaction.clear();
        mCell.clear();
        mSound.clear();
        mResponse.clear();
        mResultScript.clear();
        mFactionLess = false;
        mQuestStatus = QS_None;
    }
}
