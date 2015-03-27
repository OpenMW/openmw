#include "loadlevlist.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

namespace ESM
{

    void LevelledListBase::load(ESMReader &esm)
    {
        esm.getHNT(mFlags, "DATA");
        esm.getHNT(mChanceNone, "NNAM");

        if (esm.isNextSub("INDX"))
        {
            int len;
            esm.getHT(len);
            mList.resize(len);
        }
        else
        {
            // Original engine ignores rest of the record, even if there are items following
            mList.clear();
            esm.skipRecord();
            return;
        }

        // If this levelled list was already loaded by a previous content file,
        // we overwrite the list. Merging lists should probably be left to external tools,
        // with the limited amount of information there is in the records, all merging methods
        // will be flawed in some way. For a proper fix the ESM format would have to be changed
        // to actually track list changes instead of including the whole list for every file
        // that does something with that list.

        for (size_t i = 0; i < mList.size(); i++)
        {
            LevelItem &li = mList[i];
            li.mId = esm.getHNString(mRecName);
            esm.getHNT(li.mLevel, "INTV");
        }
    }
    void LevelledListBase::save(ESMWriter &esm) const
    {
        esm.writeHNT("DATA", mFlags);
        esm.writeHNT("NNAM", mChanceNone);
        esm.writeHNT<int>("INDX", mList.size());

        for (std::vector<LevelItem>::const_iterator it = mList.begin(); it != mList.end(); ++it)
        {
            esm.writeHNCString(mRecName, it->mId);
            esm.writeHNT("INTV", it->mLevel);
        }
    }

    void LevelledListBase::blank()
    {
        mFlags = 0;
        mChanceNone = 0;
        mList.clear();
    }

    unsigned int CreatureLevList::sRecordId = REC_LEVC;

    unsigned int ItemLevList::sRecordId = REC_LEVI;
}
