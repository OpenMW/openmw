#include "loaddial.hpp"

#include <components/debug/debuglog.hpp>

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace ESM
{

    void Dialogue::load(ESMReader &esm, bool &isDeleted)
    {
        loadId(esm);
        loadData(esm, isDeleted);
    }

    void Dialogue::loadId(ESMReader &esm)
    {
        mId = esm.getHNString("NAME");
    }

    void Dialogue::loadData(ESMReader &esm, bool &isDeleted)
    {
        isDeleted = false;

        while (esm.hasMoreSubs())
        {
            esm.getSubName();
            switch (esm.retSubName().toInt())
            {
                case fourCC("DATA"):
                {
                    esm.getSubHeader();
                    int size = esm.getSubSize();
                    if (size == 1)
                    {
                        esm.getT(mType);
                    }
                    else
                    {
                        esm.skip(size);
                    }
                    break;
                }
                case SREC_DELE:
                    esm.skipHSub();
                    mType = Unknown;
                    isDeleted = true;
                    break;
                default:
                    esm.fail("Unknown subrecord");
                    break;
            }
        }
    }

    void Dialogue::save(ESMWriter &esm, bool isDeleted) const
    {
        esm.writeHNCString("NAME", mId);
        if (isDeleted)
        {
            esm.writeHNString("DELE", "", 3);
        }
        else
        {
            esm.writeHNT("DATA", mType);
        }
    }

    void Dialogue::blank()
    {
        mInfo.clear();
    }

    void Dialogue::readInfo(ESMReader &esm, bool merge)
    {
        DialInfo info;
        bool isDeleted = false;
        info.load(esm, isDeleted);

        if (!merge || mInfo.empty())
        {
            mLookup[info.mId] = std::make_pair(mInfo.insert(mInfo.end(), info), isDeleted);
            return;
        }

        LookupMap::iterator lookup = mLookup.find(info.mId);

        if (lookup != mLookup.end())
        {
            auto it = lookup->second.first;
            // Since the new version of this record may have changed the next/prev linked list connection, we need to re-insert the record
            mInfo.erase(it);
            mLookup.erase(lookup);
        }

        if (!info.mPrev.empty())
        {
            lookup = mLookup.find(info.mPrev);
            if (lookup != mLookup.end())
            {
                auto it = lookup->second.first;

                mLookup[info.mId] = std::make_pair(mInfo.insert(++it, info), isDeleted);
            }
            else
                mLookup[info.mId] = std::make_pair(mInfo.insert(mInfo.end(), info), isDeleted);
        }
        else
            mLookup[info.mId] = std::make_pair(mInfo.insert(mInfo.begin(), info), isDeleted);
    }

    void Dialogue::clearDeletedInfos()
    {
        LookupMap::const_iterator current = mLookup.begin();
        LookupMap::const_iterator end = mLookup.end();
        for (; current != end; ++current)
        {
            if (current->second.second)
            {
                mInfo.erase(current->second.first);
            }
        }
        mLookup.clear();
    }
}
