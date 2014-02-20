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
    // TODO: use std::move
    if (!merge || mInfo.empty() || info.mNext.empty())
    {
        mInfo.push_back(info);
        return;
    }
    if (info.mPrev.empty())
    {
        mInfo.push_front(info);
        return;
    }
    int i=0;
    for (ESM::Dialogue::InfoContainer::iterator it = mInfo.begin(); it != mInfo.end(); ++it)
    {
        if (info.mPrev == it->mId)
        {
            mInfo.insert(++it, info);
            return;
        }
        if (info.mNext == it->mId)
        {
            mInfo.insert(it, info);
            return;
        }
        if (info.mId == it->mId)
        {
            *it = info;
            return;
        }
        ++i;
    }
    std::cerr << "Failed to insert info " << info.mId << std::endl;
}

}
