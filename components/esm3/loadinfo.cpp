#include "loadinfo.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace ESM
{
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
            switch (esm.retSubName().toInt())
            {
                case fourCC("DATA"):
                    esm.getHTSized<12>(mData);
                    break;
                case fourCC("ONAM"):
                    mActor = esm.getHString();
                    break;
                case fourCC("RNAM"):
                    mRace = esm.getHString();
                    break;
                case fourCC("CNAM"):
                    mClass = esm.getHString();
                    break;
                case fourCC("FNAM"):
                {
                    mFaction = esm.getHString();
                    if (mFaction == "FFFF")
                    {
                        mFactionLess = true;
                    }
                    break;
                }
                case fourCC("ANAM"):
                    mCell = esm.getHString();
                    break;
                case fourCC("DNAM"):
                    mPcFaction = esm.getHString();
                    break;
                case fourCC("SNAM"):
                    mSound = esm.getHString();
                    break;
                case SREC_NAME:
                    mResponse = esm.getHString();
                    break;
                case fourCC("SCVR"):
                {
                    SelectStruct ss;
                    ss.mSelectRule = esm.getHString();
                    ss.mValue.read(esm, Variant::Format_Info);
                    mSelects.push_back(ss);
                    break;
                }
                case fourCC("BNAM"):
                    mResultScript = esm.getHString();
                    break;
                case fourCC("QSTN"):
                    mQuestStatus = QS_Name;
                    esm.skipRecord();
                    break;
                case fourCC("QSTF"):
                    mQuestStatus = QS_Finished;
                    esm.skipRecord();
                    break;
                case fourCC("QSTR"):
                    mQuestStatus = QS_Restart;
                    esm.skipRecord();
                    break;
                case SREC_DELE:
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
        mData = {};

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
