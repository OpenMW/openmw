#ifndef OPENMW_ESM_CONT_H
#define OPENMW_ESM_CONT_H

#include <string>
#include <vector>

#include "components/esm/defs.hpp"
#include "components/esm/refid.hpp"

namespace ESM
{

    class ESMReader;
    class ESMWriter;

    /*
     * Container definition
     */

    struct ContItem
    {
        int32_t mCount{ 0 };
        ESM::RefId mItem;
    };

    /// InventoryList, NPCO subrecord
    struct InventoryList
    {
        std::vector<ContItem> mList;

        /// Load one item, assumes subrecord name is already read
        void add(ESMReader& esm);

        void save(ESMWriter& esm) const;
    };

    struct Container
    {
        constexpr static RecNameInts sRecordId = REC_CONT;

        /// Return a string descriptor for this record type. Currently used for debugging / error logs only.
        static std::string_view getRecordType() { return "Container"; }

        enum Flags
        {
            Organic = 1, // Objects cannot be placed in this container
            Respawn = 2, // Respawns after 4 months
            Unknown = 8
        };

        uint32_t mRecordFlags;
        RefId mId, mScript;
        std::string mName, mModel;

        float mWeight; // Not sure, might be max total weight allowed?
        int32_t mFlags;
        InventoryList mInventory;

        void load(ESMReader& esm, bool& isDeleted);
        void save(ESMWriter& esm, bool isDeleted = false) const;

        void blank();
        ///< Set record to default state (does not touch the ID).
    };
}
#endif
