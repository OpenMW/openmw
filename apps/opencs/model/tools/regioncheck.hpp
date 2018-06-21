#ifndef CSM_TOOLS_REGIONCHECK_H
#define CSM_TOOLS_REGIONCHECK_H

#include <components/esm/loadregn.hpp>

#include "../world/idcollection.hpp"

#include "../doc/stage.hpp"

namespace CSMTools
{
    /// \brief VerifyStage: make sure that region records are internally consistent
    class RegionCheckStage : public CSMDoc::Stage
    {
            const CSMWorld::IdCollection<ESM::Region>& mRegions;
            bool mIgnoreBaseRecords;

        public:

            RegionCheckStage (const CSMWorld::IdCollection<ESM::Region>& regions);

            virtual int setup();
            ///< \return number of steps

            virtual void perform (int stage, CSMDoc::Messages& messages);
            ///< Messages resulting from this tage will be appended to \a messages.
    };
}

#endif
