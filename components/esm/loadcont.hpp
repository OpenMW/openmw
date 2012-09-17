#ifndef _ESM_CONT_H
#define _ESM_CONT_H

#include <string>
#include <vector>

#include "record.hpp"
#include "esm_common.hpp"

namespace ESM
{

/*
 * Container definition
 */

struct ContItem
{
    int mCount;
    NAME32 mItem;
};

struct InventoryList
{
    std::vector<ContItem> mList;

    void load(ESMReader &esm);
    void save(ESMWriter &esm);
};

struct Container : public Record
{
    enum Flags
    {
        Organic = 1, // Objects cannot be placed in this container
        Respawn = 2, // Respawns after 4 months
        Unknown = 8
    };

    std::string mName, mModel, mScript;

    float mWeight; // Not sure, might be max total weight allowed?
    int mFlags;
    InventoryList mInventory;

    void load(ESMReader &esm);
    void save(ESMWriter &esm);

    int getName() { return REC_CONT; }
};
}
#endif
