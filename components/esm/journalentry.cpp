#include "journalentry.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

void ESM::JournalEntry::load (ESMReader &esm)
{
    esm.getHNOT (mType, "JETY");
    mTopic = esm.getHNString ("YETO");
    mInfo = esm.getHNString ("YEIN");
    mText = esm.getHNString ("TEXT");

    if (mType==Type_Journal)
    {
        esm.getHNT (mDay, "JEDA");
        esm.getHNT (mMonth, "JEMO");
        esm.getHNT (mDayOfMonth, "JEDM");
    }
    else if (mType==Type_Topic)
        mActorName = esm.getHNOString("ACT_");
}

void ESM::JournalEntry::save (ESMWriter &esm) const
{
    esm.writeHNT ("JETY", mType);
    esm.writeHNString ("YETO", mTopic);
    esm.writeHNString ("YEIN", mInfo);
    esm.writeHNString ("TEXT", mText);

    if (mType==Type_Journal)
    {
        esm.writeHNT ("JEDA", mDay);
        esm.writeHNT ("JEMO", mMonth);
        esm.writeHNT ("JEDM", mDayOfMonth);
    }
    else if (mType==Type_Topic)
        esm.writeHNString ("ACT_", mActorName);
}
