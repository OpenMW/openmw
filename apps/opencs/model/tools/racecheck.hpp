#ifndef CSM_TOOLS_RACECHECK_H
#define CSM_TOOLS_RACECHECK_H

#include <components/esm/loadrace.hpp>

#include "../world/idcollection.hpp"

#include "../doc/stage.hpp"

namespace CSMTools
{
    /// \brief VerifyStage: make sure that race records are internally consistent
    class RaceCheckStage : public CSMDoc::Stage
    {
            const CSMWorld::IdCollection<ESM::Race>& mRaces;
            bool mPlayable;

            void performPerRecord (int stage, CSMDoc::Messages& messages);

            void performFinal (CSMDoc::Messages& messages);

        public:

            RaceCheckStage (const CSMWorld::IdCollection<ESM::Race>& races);

            virtual int setup();
            ///< \return number of steps

            virtual void perform (int stage, CSMDoc::Messages& messages);
            ///< Messages resulting from this tage will be appended to \a messages.
    };
}

#endif
