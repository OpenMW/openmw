#ifndef OPENMW_ESM_MAPPINGS_H
#define OPENMW_ESM_MAPPINGS_H

#include <string>

#include <components/esm3/loadarmo.hpp>
#include <components/esm3/loadbody.hpp>

namespace ESM
{
    ESM::BodyPart::MeshPart getMeshPart(ESM::PartReferenceType type);
    std::string getBoneName(ESM::PartReferenceType type);
    std::string getMeshFilter(ESM::PartReferenceType type);
}

#endif
