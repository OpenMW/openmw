#ifndef CSM_TOOLS_FACTIONCHECK_H
#define CSM_TOOLS_FACTIONCHECK_H

#include <components/esm/loadfact.hpp>

#include "../world/idcollection.hpp"

#include "../doc/stage.hpp"

namespace CSMTools
{
    /// \brief VerifyStage: make sure that faction records are internally consistent
    class FactionCheckStage : public CSMDoc::Stage
    {
            const CSMWorld::IdCollection<ESM::Faction>& mFactions;
            bool mIgnoreBaseRecords;

        public:

            FactionCheckStage (const CSMWorld::IdCollection<ESM::Faction>& factions);

            virtual int setup();
            ///< \return number of steps

            virtual void perform (int stage, CSMDoc::Messages& messages);
            ///< Messages resulting from this tage will be appended to \a messages.
    };
}

#endif
