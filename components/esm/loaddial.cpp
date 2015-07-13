#include "loaddial.hpp"

#include <iostream>

#include <cstdint>

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"
#include "util.hpp"

namespace ESM
{
    unsigned int Dialogue::sRecordId = REC_DIAL;

    Dialogue::Dialogue()
        : mIsDeleted(false)
    {}

    void Dialogue::load(ESMReader &esm)
    {
        loadId(esm);
        loadData(esm);
    }

    void Dialogue::loadId(ESMReader &esm)
    {
        mIsDeleted = false;
        mId = esm.getHNString("NAME");
    }

    void Dialogue::loadData(ESMReader &esm)
    {
        esm.getSubNameIs("DATA");
        esm.getSubHeader();
        int si = esm.getSubSize();
        if (si == 1)
            esm.getT(mType);
        else if (si == 4) // The dialogue is deleted
        {
            int32_t empty;
            esm.getT(empty); // Skip an empty DATA
            mIsDeleted = readDeleSubRecord(esm);
            mType = Unknown;
        }
        else
            esm.fail("Unknown sub record size");
    }

    void Dialogue::save(ESMWriter &esm) const
    {
        esm.writeHNCString("NAME", mId);
        if (mIsDeleted)
        {
            esm.writeHNT("DATA", static_cast<int32_t>(0));
            writeDeleSubRecord(esm);
        }
        else
        {
            esm.writeHNT("DATA", mType);
        }
    }

    void Dialogue::blank()
    {
        mInfo.clear();
        mIsDeleted = false;
    }

    void Dialogue::readInfo(ESMReader &esm, bool merge)
    {
        ESM::DialInfo info;
        info.loadId(esm);

        if (!merge || mInfo.empty())
        {
            info.loadInfo(esm);
            mLookup[info.mId] = mInfo.insert(mInfo.end(), info);
            return;
        }

        ESM::Dialogue::InfoContainer::iterator it = mInfo.end();

        std::map<std::string, ESM::Dialogue::InfoContainer::iterator>::iterator lookup;
        lookup = mLookup.find(info.mId);

        if (lookup != mLookup.end())
        {
            it = lookup->second;

            // Merge with existing record. Only the subrecords that are present in
            // the new record will be overwritten.
            it->loadInfo(esm);
            info = *it;

            // Since the record merging may have changed the next/prev linked list connection, we need to re-insert the record
            mInfo.erase(it);
            mLookup.erase(lookup);
        }
        else
        {
            info.loadInfo(esm);
        }

        if (info.mNext.empty())
        {
            mLookup[info.mId] = mInfo.insert(mInfo.end(), info);
            return;
        }
        if (info.mPrev.empty())
        {
            mLookup[info.mId] = mInfo.insert(mInfo.begin(), info);
            return;
        }

        lookup = mLookup.find(info.mPrev);
        if (lookup != mLookup.end())
        {
            it = lookup->second;

            mLookup[info.mId] = mInfo.insert(++it, info);
            return;
        }

        lookup = mLookup.find(info.mNext);
        if (lookup != mLookup.end())
        {
            it = lookup->second;

            mLookup[info.mId] = mInfo.insert(it, info);
            return;
        }

        std::cerr << "Failed to insert info " << info.mId << std::endl;
    }

    void Dialogue::clearDeletedInfos()
    {
        for (InfoContainer::iterator it = mInfo.begin(); it != mInfo.end(); )
        {
            if (it->mIsDeleted || it->mQuestStatus == DialInfo::QS_Deleted)
                it = mInfo.erase(it);
            else
                ++it;
        }
    }
}
