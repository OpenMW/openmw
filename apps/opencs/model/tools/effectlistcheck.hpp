#ifndef CSM_TOOLS_EFFECTLISTCHECK_H
#define CSM_TOOLS_EFFECTLISTCHECK_H

#include <vector>

namespace ESM
{
    struct IndexedENAMstruct;
    struct Ingredient;
}

namespace CSMDoc
{
    class Messages;
}

namespace CSMWorld
{
    class UniversalId;
}

namespace CSMTools
{
    void effectListCheck(
        const std::vector<ESM::IndexedENAMstruct>& list, CSMDoc::Messages& messages, const CSMWorld::UniversalId& id);
    void ingredientEffectListCheck(
        const ESM::Ingredient& ingredient, CSMDoc::Messages& messages, const CSMWorld::UniversalId& id);
}

#endif
