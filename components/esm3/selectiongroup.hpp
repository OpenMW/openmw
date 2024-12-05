#ifndef COMPONENTS_ESM_SELECTIONGROUP_H
#define COMPONENTS_ESM_SELECTIONGROUP_H

#include <string>

#include "components/esm/defs.hpp"
#include "components/esm/refid.hpp"

namespace ESM
{
    class ESMReader;
    class ESMWriter;

    struct SelectionGroup
    {
        constexpr static RecNameInts sRecordId = REC_SELG;

        static constexpr std::string_view getRecordType() { return "SelectionGroup"; }

        uint32_t mRecordFlags = 0;

        RefId mId;

        std::vector<std::string> selectedInstances;

        void load(ESMReader& esm, bool& isDeleted);
        void save(ESMWriter& esm, bool isDeleted = false) const;

        /// Set record to default state (does not touch the ID).
        void blank();
    };
}

#endif
