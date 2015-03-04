#include "loaddial.hpp"

#include <iostream>

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

namespace ESM
{
    unsigned int Dialogue::sRecordId = REC_DIAL;

void Dialogue::load(ESMReader &esm)
{
    esm.getSubNameIs("DATA");
    esm.getSubHeader();
    int si = esm.getSubSize();
    if (si == 1)
        esm.getT(mType);
    else if (si == 4)
    {
        // These are just markers, their values are not used.
        int i;
        esm.getT(i);
        esm.getHNT(i, "DELE");
        mType = Deleted;
    }
    else
        esm.fail("Unknown sub record size");
}

void Dialogue::save(ESMWriter &esm) const
{
    if (mType != Deleted)
        esm.writeHNT("DATA", mType);
    else
    {
        esm.writeHNT("DATA", (int)1);
        esm.writeHNT("DELE", (int)1);
    }
}

void Dialogue::blank()
{
    mInfo.clear();
}

void Dialogue::readInfo(ESMReader &esm, bool merge)
{
    const std::string& id = esm.getHNOString("INAM");

    if (!merge || mInfo.empty())
    {
        ESM::DialInfo info;
        info.mId = id;
        info.load(esm);
        mLookup[id] = mInfo.insert(mInfo.end(), info);
        return;
    }

    ESM::Dialogue::InfoContainer::iterator it = mInfo.end();

    std::map<std::string, ESM::Dialogue::InfoContainer::iterator>::iterator lookup;

    lookup = mLookup.find(id);

    ESM::DialInfo info;
    if (lookup != mLookup.end())
    {
        it = lookup->second;

        // Merge with existing record. Only the subrecords that are present in
        // the new record will be overwritten.
        it->load(esm);
        info = *it;

        // Since the record merging may have changed the next/prev linked list connection, we need to re-insert the record
        mInfo.erase(it);
        mLookup.erase(lookup);
    }
    else
    {
        info.mId = id;
        info.load(esm);
    }

    if (info.mNext.empty())
    {
        mLookup[id] = mInfo.insert(mInfo.end(), info);
        return;
    }
    if (info.mPrev.empty())
    {
        mLookup[id] = mInfo.insert(mInfo.begin(), info);
        return;
    }

    lookup = mLookup.find(info.mPrev);
    if (lookup != mLookup.end())
    {
        it = lookup->second;

        mLookup[id] = mInfo.insert(++it, info);
        return;
    }

    lookup = mLookup.find(info.mNext);
    if (lookup != mLookup.end())
    {
        it = lookup->second;

        mLookup[id] = mInfo.insert(it, info);
        return;
    }

    std::cerr << "Failed to insert info " << id << std::endl;
}

void Dialogue::clearDeletedInfos()
{
    for (InfoContainer::iterator it = mInfo.begin(); it != mInfo.end(); )
    {
        if (it->mQuestStatus == DialInfo::QS_Deleted)
            it = mInfo.erase(it);
        else
            ++it;
    }
}

}
