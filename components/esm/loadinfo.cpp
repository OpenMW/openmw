#include "loadinfo.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

namespace ESM
{
    unsigned int DialInfo::sRecordId = REC_INFO;

void DialInfo::load(ESMReader &esm)
{
    mQuestStatus = QS_None;
    mFactionLess = false;

    mPrev = esm.getHNString("PNAM");
    mNext = esm.getHNString("NNAM");

    // Since there's no way to mark selects as "deleted", we have to clear the SelectStructs from all previous loadings
    mSelects.clear();

    // Not present if deleted
    if (esm.isNextSub("DATA")) {
        esm.getHT(mData, 12);
    }

    if (!esm.hasMoreSubs())
        return;

    // What follows is somewhat spaghetti-ish, but it's worth if for
    // an extra speedup. INFO is by far the most common record type.

    // subName is a reference to the original, so it changes whenever
    // a new sub name is read. esm.isEmptyOrGetName() will get the
    // next name for us, or return true if there are no more records.
    esm.getSubName();
    const NAME &subName = esm.retSubName();

    if (subName.val == REC_ONAM)
    {
        mActor = esm.getHString();
        if (esm.isEmptyOrGetName())
            return;
    }
    if (subName.val == REC_RNAM)
    {
        mRace = esm.getHString();
        if (esm.isEmptyOrGetName())
            return;
    }
    if (subName.val == REC_CNAM)
    {
        mClass = esm.getHString();
        if (esm.isEmptyOrGetName())
            return;
    }

    if (subName.val == REC_FNAM)
    {
        mFaction = esm.getHString();
        if (mFaction == "FFFF")
            mFactionLess = true;
        if (esm.isEmptyOrGetName())
            return;
    }
    if (subName.val == REC_ANAM)
    {
        mCell = esm.getHString();
        if (esm.isEmptyOrGetName())
            return;
    }
    if (subName.val == REC_DNAM)
    {
        mPcFaction = esm.getHString();
        if (esm.isEmptyOrGetName())
            return;
    }
    if (subName.val == REC_SNAM)
    {
        mSound = esm.getHString();
        if (esm.isEmptyOrGetName())
            return;
    }
    if (subName.val == REC_NAME)
    {
        mResponse = esm.getHString();
        if (esm.isEmptyOrGetName())
            return;
    }

    while (subName.val == REC_SCVR)
    {
        SelectStruct ss;

        ss.mSelectRule = esm.getHString();

        ss.mValue.read (esm, Variant::Format_Info);

        mSelects.push_back(ss);

        if (esm.isEmptyOrGetName())
            return;
    }

    if (subName.val == REC_BNAM)
    {
        mResultScript = esm.getHString();
        if (esm.isEmptyOrGetName())
            return;
    }

    if (subName.val == REC_QSTN)
        mQuestStatus = QS_Name;
    else if (subName.val == REC_QSTF)
        mQuestStatus = QS_Finished;
    else if (subName.val == REC_QSTR)
        mQuestStatus = QS_Restart;
    else if (subName.val == REC_DELE)
        mQuestStatus = QS_Deleted;
    else
        esm.fail(
                "Don't know what to do with " + subName.toString()
                        + " in INFO " + mId);

    if (mQuestStatus != QS_None)
        // Skip rest of record
        esm.skipRecord();
}

void DialInfo::save(ESMWriter &esm) const
{
    esm.writeHNCString("PNAM", mPrev);
    esm.writeHNCString("NNAM", mNext);
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
    case QS_Deleted: esm.writeHNT("DELE", '\1'); break;
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
