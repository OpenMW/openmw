#include "journalentry.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace ESM
{

    void JournalEntry::load(ESMReader& esm)
    {
        esm.getHNOT(mType, "JETY");
        mTopic = ESM::RefId::stringRefId(esm.getHNString("YETO"));
        mInfo = ESM::RefId::stringRefId(esm.getHNString("YEIN"));
        mText = esm.getHNString("TEXT");

        if (mType == Type_Journal)
        {
            esm.getHNT(mDay, "JEDA");
            esm.getHNT(mMonth, "JEMO");
            esm.getHNT(mDayOfMonth, "JEDM");
        }
        else if (mType == Type_Topic)
            mActorName = esm.getHNOString("ACT_");
    }

    void JournalEntry::save(ESMWriter& esm) const
    {
        esm.writeHNT("JETY", mType);
        esm.writeHNString("YETO", mTopic.getRefIdString());
        esm.writeHNString("YEIN", mInfo.getRefIdString());
        esm.writeHNString("TEXT", mText);

        if (mType == Type_Journal)
        {
            esm.writeHNT("JEDA", mDay);
            esm.writeHNT("JEMO", mMonth);
            esm.writeHNT("JEDM", mDayOfMonth);
        }
        else if (mType == Type_Topic)
            esm.writeHNString("ACT_", mActorName);
    }

}
