#ifndef CSM_TOOLS_FACTIONCHECK_H
#define CSM_TOOLS_FACTIONCHECK_H

#include <components/esm/loadfact.hpp>

#include "../world/idcollection.hpp"

#include "stage.hpp"

namespace CSMTools
{
    /// \brief VerifyStage: make sure that faction records are internally consistent
    class FactionCheckStage : public Stage
    {
            const CSMWorld::IdCollection<ESM::Faction>& mFactions;

        public:

            FactionCheckStage (const CSMWorld::IdCollection<ESM::Faction>& factions);

            virtual int setup();
            ///< \return number of steps

            virtual void perform (int stage, std::vector<std::string>& messages);
            ///< Messages resulting from this tage will be appended to \a messages.
    };
}

#endif
