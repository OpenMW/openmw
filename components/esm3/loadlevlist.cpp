#include "loadlevlist.hpp"

#include "components/esm/defs.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace ESM
{
    void LevelledListBase::load(ESMReader& esm, NAME recName, bool& isDeleted)
    {
        isDeleted = false;
        mRecordFlags = esm.getRecordFlags();

        bool hasName = false;
        bool hasList = false;
        while (esm.hasMoreSubs())
        {
            esm.getSubName();
            switch (esm.retSubName().toInt())
            {
                case SREC_NAME:
                    mId = esm.getRefId();
                    hasName = true;
                    break;
                case fourCC("DATA"):
                    esm.getHT(mFlags);
                    break;
                case fourCC("NNAM"):
                    esm.getHT(mChanceNone);
                    break;
                case fourCC("INDX"):
                {
                    uint32_t length = 0;
                    esm.getHT(length);
                    mList.resize(length);

                    // If this levelled list was already loaded by a previous content file,
                    // we overwrite the list. Merging lists should probably be left to external tools,
                    // with the limited amount of information there is in the records, all merging methods
                    // will be flawed in some way. For a proper fix the ESM format would have to be changed
                    // to actually track list changes instead of including the whole list for every file
                    // that does something with that list.
                    for (size_t i = 0; i < mList.size(); i++)
                    {
                        LevelItem& li = mList[i];
                        li.mId = esm.getHNRefId(recName);
                        esm.getHNT(li.mLevel, "INTV");
                    }

                    hasList = true;
                    break;
                }
                case SREC_DELE:
                    esm.skipHSub();
                    isDeleted = true;
                    break;
                default:
                {
                    if (!hasList)
                    {
                        // Original engine ignores rest of the record, even if there are items following
                        mList.clear();
                        esm.skipRecord();
                    }
                    else
                    {
                        esm.fail("Unknown subrecord");
                    }
                    break;
                }
            }
        }

        if (!hasName)
            esm.fail("Missing NAME subrecord");
    }

    void LevelledListBase::save(ESMWriter& esm, NAME recName, bool isDeleted) const
    {
        esm.writeHNCRefId("NAME", mId);

        if (isDeleted)
        {
            esm.writeHNString("DELE", "", 3);
            return;
        }

        esm.writeHNT("DATA", mFlags);
        esm.writeHNT("NNAM", mChanceNone);
        esm.writeHNT("INDX", static_cast<uint32_t>(mList.size()));

        for (const auto& item : mList)
        {
            esm.writeHNCRefId(recName, item.mId);
            esm.writeHNT("INTV", item.mLevel);
        }
    }

    void LevelledListBase::blank()
    {
        mRecordFlags = 0;
        mFlags = 0;
        mChanceNone = 0;
        mList.clear();
    }
}
