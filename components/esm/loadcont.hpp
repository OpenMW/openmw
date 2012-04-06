#ifndef _ESM_CONT_H
#define _ESM_CONT_H

#include "esm_reader.hpp"
#include "esm_writer.hpp"

namespace ESM
{

/*
 * Container definition
 */

struct ContItem
{
    int count;
    NAME32 item;
};

struct InventoryList
{
    std::vector<ContItem> list;

    void load(ESMReader &esm);
    void save(ESMWriter &esm);
};

struct Container
{
    enum Flags
    {
        Organic = 1, // Objects cannot be placed in this container
        Respawn = 2, // Respawns after 4 months
        Unknown = 8
    };

    std::string name, model, script;

    float weight; // Not sure, might be max total weight allowed?
    int flags;
    InventoryList inventory;

    void load(ESMReader &esm);
    void save(ESMWriter &esm);
};
}
#endif
