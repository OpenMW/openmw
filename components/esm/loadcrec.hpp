#ifndef OPENMW_ESM_CREC_H
#define OPENMW_ESM_CREC_H

// TODO create implementation files and remove this one
#include "esmreader.hpp"

namespace ESM {

class ESMReader;
class ESMWriter;

/* These two are only used in save games. They are not decoded yet.
 */

/// Changes a creature
struct LoadCREC
{
    void load(ESMReader &esm)
    {
      esm.skipRecord();
    }

    void save(ESMWriter &esm)
    {
    }
};

/// Changes an item list / container
struct LoadCNTC
{
    void load(ESMReader &esm)
    {
      esm.skipRecord();
    }

    void save(ESMWriter &esm)
    {
    }
};
}
#endif
