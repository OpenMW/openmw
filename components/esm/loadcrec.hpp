#ifndef OPENMW_ESM_CREC_H
#define OPENMW_ESM_CREC_H

#include "record.hpp"

// TODO create implementation files and remove this one
#include "esm_reader.hpp"

namespace ESM {

/* These two are only used in save games. They are not decoded yet.
 */

/// Changes a creature
struct LoadCREC : public Record
{
    void load(ESMReader &esm)
    {
      esm.skipRecord();
    }

    void save(ESMWriter &esm)
    {
    }

    int getName() { return REC_CREC; }
};

/// Changes an item list / container
struct LoadCNTC : public Record
{
    void load(ESMReader &esm)
    {
      esm.skipRecord();
    }

    void save(ESMWriter &esm)
    {
    }

    int getName() { return REC_CNTC; }
};
}
#endif
