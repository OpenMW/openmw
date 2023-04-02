#include "loadstat.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

#include <components/settings/settings.hpp>
#include <components/misc/strings/lower.hpp>

namespace ESM
{
    void Static::load(ESMReader& esm, bool& isDeleted)
    {
        isDeleted = false;
        mRecordFlags = esm.getRecordFlags();
        // bool isBlocked = (mRecordFlags & FLAG_Blocked) != 0;
        // bool isPersistent = (mRecordFlags & FLAG_Persistent) != 0;

        bool hasName = false;
        while (esm.hasMoreSubs())
        {
            esm.getSubName();
            switch (esm.retSubName().toInt())
            {
                case SREC_NAME:
                    mId = esm.getRefId();
                    hasName = true;
                    break;
                case fourCC("MODL"):
                    mModel = esm.getHString();
                    break;
                case SREC_DELE:
                    esm.skipHSub();
                    isDeleted = true;
                    break;
                default:
                    esm.fail("Unknown subrecord");
                    break;
            }
        }

        if (!hasName)
            esm.fail("Missing NAME subrecord");

        static const bool groundcoverEnabled = Settings::Manager::getBool("paging", "Groundcover");
        if(groundcoverEnabled)
        {
            std::string mesh = Misc::StringUtils::lowerCase (mModel);
            if (mesh.find("grass\\") == 0)
                mIsGroundcover = true;
        }
    }
    void Static::save(ESMWriter& esm, bool isDeleted) const
    {
        esm.writeHNCRefId("NAME", mId);
        if (isDeleted)
        {
            esm.writeHNString("DELE", "", 3);
        }
        else
        {
            esm.writeHNCString("MODL", mModel);
        }
    }

    void Static::blank()
    {
        mRecordFlags = 0;
        mModel.clear();
    }
}
