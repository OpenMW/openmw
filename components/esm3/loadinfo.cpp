#include "loadinfo.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace ESM
{
    void DialInfo::load(ESMReader& esm, bool& isDeleted)
    {
        mId = ESM::RefId::stringRefId(esm.getHNString("INAM"));

        isDeleted = false;

        mQuestStatus = QS_None;
        mFactionLess = false;

        mPrev = ESM::RefId::stringRefId(esm.getHNString("PNAM"));
        mNext = ESM::RefId::stringRefId(esm.getHNString("NNAM"));

        while (esm.hasMoreSubs())
        {
            esm.getSubName();
            switch (esm.retSubName().toInt())
            {
                case fourCC("DATA"):
                    esm.getHTSized<12>(mData);
                    break;
                case fourCC("ONAM"):
                    mActor = esm.getRefId();
                    break;
                case fourCC("RNAM"):
                    mRace = esm.getRefId();
                    break;
                case fourCC("CNAM"):
                    mClass = esm.getRefId();
                    break;
                case fourCC("FNAM"):
                {
                    mFaction = esm.getRefId();
                    if (mFaction.getRefIdString() == "FFFF")
                    {
                        mFactionLess = true;
                    }
                    break;
                }
                case fourCC("ANAM"):
                    mCell = esm.getRefId();
                    break;
                case fourCC("DNAM"):
                    mPcFaction = esm.getRefId();
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

    void DialInfo::save(ESMWriter& esm, bool isDeleted) const
    {
        esm.writeHNCString("INAM", mId.getRefIdString());
        esm.writeHNCString("PNAM", mPrev.getRefIdString());
        esm.writeHNCString("NNAM", mNext.getRefIdString());

        if (isDeleted)
        {
            esm.writeHNString("DELE", "", 3);
            return;
        }

        esm.writeHNT("DATA", mData, 12);
        esm.writeHNOCString("ONAM", mActor.getRefIdString());
        esm.writeHNOCString("RNAM", mRace.getRefIdString());
        esm.writeHNOCString("CNAM", mClass.getRefIdString());
        esm.writeHNOCString("FNAM", mFaction.getRefIdString());
        esm.writeHNOCString("ANAM", mCell.getRefIdString());
        esm.writeHNOCString("DNAM", mPcFaction.getRefIdString());
        esm.writeHNOCString("SNAM", mSound);
        esm.writeHNOString("NAME", mResponse);

        for (std::vector<SelectStruct>::const_iterator it = mSelects.begin(); it != mSelects.end(); ++it)
        {
            esm.writeHNString("SCVR", it->mSelectRule);
            it->mValue.write(esm, Variant::Format_Info);
        }

        esm.writeHNOString("BNAM", mResultScript);

        switch (mQuestStatus)
        {
            case QS_Name:
                esm.writeHNT("QSTN", '\1');
                break;
            case QS_Finished:
                esm.writeHNT("QSTF", '\1');
                break;
            case QS_Restart:
                esm.writeHNT("QSTR", '\1');
                break;
            default:
                break;
        }
    }

    void DialInfo::blank()
    {
        mData = {};

        mSelects.clear();
        mPrev = ESM::RefId::sEmpty;
        mNext = ESM::RefId::sEmpty;
        mActor = ESM::RefId::sEmpty;
        mRace = ESM::RefId::sEmpty;
        mClass = ESM::RefId::sEmpty;
        mFaction = ESM::RefId::sEmpty;
        mPcFaction = ESM::RefId::sEmpty;
        mCell = ESM::RefId::sEmpty;
        mSound.clear();
        mResponse.clear();
        mResultScript.clear();
        mFactionLess = false;
        mQuestStatus = QS_None;
    }
}
