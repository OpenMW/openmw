#include "selectiongroup.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace ESM
{
    void SelectionGroup::load(ESMReader& esm, bool& isDeleted)
    {

        while (esm.hasMoreSubs())
        {
            esm.getSubName();
            switch (esm.retSubName().toInt())
            {
                case fourCC("SELC"):
                    mId = esm.getRefId();
                    break;
                case fourCC("SELI"):
                    selectedInstances.push_back(esm.getRefId().getRefIdString());
                    break;
                default:
                    esm.fail("Unknown subrecord");
                    break;
            }
        }
    }

    void SelectionGroup::save(ESMWriter& esm, bool isDeleted) const
    {
        esm.writeHNCRefId("SELC", mId);
        for (std::string id : selectedInstances)
            esm.writeHNCString("SELI", id);
    }

    void SelectionGroup::blank() {}

}
