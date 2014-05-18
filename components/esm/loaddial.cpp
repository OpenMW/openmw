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

void Dialogue::addInfo(const ESM::DialInfo& info, bool merge)
{
    if (!merge || mInfo.empty() || info.mNext.empty())
    {
        mLookup[info.mId] = mInfo.insert(mInfo.end(), info);
        return;
    }
    if (info.mPrev.empty())
    {
        mLookup[info.mId] = mInfo.insert(mInfo.begin(), info);
        return;
    }

    ESM::Dialogue::InfoContainer::iterator it = mInfo.end();

    std::map<std::string, ESM::Dialogue::InfoContainer::iterator>::iterator lookup;
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

    lookup = mLookup.find(info.mId);
    if (lookup != mLookup.end())
    {
        it = lookup->second;
        *it = info;
        return;
    }

    std::cerr << "Failed to insert info " << info.mId << std::endl;
}

}
