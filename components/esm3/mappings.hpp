#ifndef OPENMW_ESM_MAPPINGS_H
#define OPENMW_ESM_MAPPINGS_H

#include <string>

#include <components/esm3/loadarmo.hpp>
#include <components/esm3/loadbody.hpp>

namespace ESM
{
    BodyPart::MeshPart getMeshPart(PartReferenceType type);
    std::string getBoneName(PartReferenceType type);
    std::string getMeshFilter(PartReferenceType type);
}

#endif
