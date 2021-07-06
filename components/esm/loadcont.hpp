#ifndef OPENMW_ESM_CONT_H
#define OPENMW_ESM_CONT_H

#include <string>
#include <vector>

#include "esmcommon.hpp"

namespace ESM
{

class ESMReader;
class ESMWriter;

/*
 * Container definition
 */

struct ContItem
{
    int mCount;
    std::string mItem;
};

/// InventoryList, NPCO subrecord
struct InventoryList
{
    std::vector<ContItem> mList;

    /// Load one item, assumes subrecord name is already read
    void add(ESMReader &esm);

    void save(ESMWriter &esm) const;
};

struct Container
{
    static unsigned int sRecordId;
    /// Return a string descriptor for this record type. Currently used for debugging / error logs only.
    static std::string getRecordType() { return "Container"; }

    enum Flags
    {
        Organic = 1, // Objects cannot be placed in this container
        Respawn = 2, // Respawns after 4 months
        Unknown = 8
    };

    unsigned int mRecordFlags;
    std::string mId, mName, mModel, mScript;

    float mWeight; // Not sure, might be max total weight allowed?
    int mFlags;
    InventoryList mInventory;

    void load(ESMReader &esm, bool &isDeleted);
    void save(ESMWriter &esm, bool isDeleted = false) const;

    void blank();
    ///< Set record to default state (does not touch the ID).
};
}
#endif
