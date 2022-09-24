#ifndef CSM_TOOLS_REFERENCECHECK_H
#define CSM_TOOLS_REFERENCECHECK_H

#include "../world/idcollection.hpp"
#include "../world/refcollection.hpp"
#include "../world/refidcollection.hpp"
#include "../world/refiddata.hpp"

#include "../doc/stage.hpp"

namespace ESM
{
    struct Faction;
}

namespace CSMTools
{
    class ReferenceCheckStage : public CSMDoc::Stage
    {
    public:
        ReferenceCheckStage(const CSMWorld::RefCollection& references, const CSMWorld::RefIdCollection& referencables,
            const CSMWorld::IdCollection<CSMWorld::Cell>& cells, const CSMWorld::IdCollection<ESM::Faction>& factions);

        void perform(int stage, CSMDoc::Messages& messages) override;
        int setup() override;

    private:
        const CSMWorld::RefCollection& mReferences;
        const CSMWorld::RefIdCollection& mObjects;
        const CSMWorld::RefIdData& mDataSet;
        const CSMWorld::IdCollection<CSMWorld::Cell>& mCells;
        const CSMWorld::IdCollection<ESM::Faction>& mFactions;
        bool mIgnoreBaseRecords;
    };
}

#endif // CSM_TOOLS_REFERENCECHECK_H
