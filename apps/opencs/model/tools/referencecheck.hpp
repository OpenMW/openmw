#ifndef CSM_TOOLS_REFERENCECHECK_H
#define CSM_TOOLS_REFERENCECHECK_H

#include "../doc/state.hpp"
#include "../doc/document.hpp"

namespace CSMTools
{
    class ReferenceCheckStage : public CSMDoc::Stage
    {
        public:
            ReferenceCheckStage (const CSMWorld::RefCollection& references,
                const CSMWorld::RefIdCollection& referencables,
                const CSMWorld::IdCollection<CSMWorld::Cell>& cells,
                const CSMWorld::IdCollection<ESM::Faction>& factions);

            virtual void perform(int stage, CSMDoc::Messages& messages);
            virtual int setup();

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
