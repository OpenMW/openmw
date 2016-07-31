#ifndef OPENMW_ESM_GLOB_H
#define OPENMW_ESM_GLOB_H

#include <string>

#include "variant.hpp"

namespace ESM
{

class ESMReader;
class ESMWriter;

/*
 * Global script variables
 */

struct Global
{
    static unsigned int sRecordId;
    /// Return a string descriptor for this record type. Currently used for debugging / error logs only.
    static std::string getRecordType() { return "Global"; }

    std::string mId;
    Variant mValue;

    void load(ESMReader &esm, bool &isDeleted);
    void save(ESMWriter &esm, bool isDeleted = false) const;

    void blank();
    ///< Set record to default state (does not touch the ID).
};

bool operator== (const Global& left, const Global& right);

}
#endif
