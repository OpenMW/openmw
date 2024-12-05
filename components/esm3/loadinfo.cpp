#include "loadinfo.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

#include <components/misc/concepts.hpp>

namespace ESM
{
    template <Misc::SameAsWithoutCvref<DialInfo::DATAstruct> T>
    void decompose(T&& v, const auto& f)
    {
        char padding = 0;
        f(v.mType, v.mDisposition, v.mRank, v.mGender, v.mPCrank, padding);
    }

    void DialInfo::load(ESMReader& esm, bool& isDeleted)
    {
        mId = esm.getHNRefId("INAM");

        isDeleted = false;

        mQuestStatus = QS_None;
        mFactionLess = false;

        mPrev = esm.getHNRefId("PNAM");
        mNext = esm.getHNRefId("NNAM");

        while (esm.hasMoreSubs())
        {
            esm.getSubName();
            switch (esm.retSubName().toInt())
            {
                case fourCC("DATA"):
                    esm.getSubComposite(mData);
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
                    if (mFaction == "FFFF")
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
                    auto filter = DialogueCondition::load(esm, mId);
                    if (filter)
                        mSelects.emplace_back(std::move(*filter));
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
        esm.writeHNCRefId("INAM", mId);
        esm.writeHNCRefId("PNAM", mPrev);
        esm.writeHNCRefId("NNAM", mNext);

        if (isDeleted)
        {
            esm.writeHNString("DELE", "", 3);
            return;
        }

        esm.writeNamedComposite("DATA", mData);
        esm.writeHNOCRefId("ONAM", mActor);
        esm.writeHNOCRefId("RNAM", mRace);
        esm.writeHNOCRefId("CNAM", mClass);
        esm.writeHNOCRefId("FNAM", mFaction);
        esm.writeHNOCRefId("ANAM", mCell);
        esm.writeHNOCRefId("DNAM", mPcFaction);
        esm.writeHNOCString("SNAM", mSound);
        esm.writeHNOString("NAME", mResponse);

        for (const auto& rule : mSelects)
            rule.save(esm);

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
        mPrev = ESM::RefId();
        mNext = ESM::RefId();
        mActor = ESM::RefId();
        mRace = ESM::RefId();
        mClass = ESM::RefId();
        mFaction = ESM::RefId();
        mPcFaction = ESM::RefId();
        mCell = ESM::RefId();
        mSound.clear();
        mResponse.clear();
        mResultScript.clear();
        mFactionLess = false;
        mQuestStatus = QS_None;
    }
}
