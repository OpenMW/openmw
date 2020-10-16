#ifndef CSM_TOOLS_BIRTHSIGNCHECK_H
#define CSM_TOOLS_BIRTHSIGNCHECK_H

#include <components/esm/loadbsgn.hpp>

#include "../world/idcollection.hpp"
#include "../world/resources.hpp"

#include "../doc/stage.hpp"

namespace CSMTools
{
    /// \brief VerifyStage: make sure that birthsign records are internally consistent
    class BirthsignCheckStage : public CSMDoc::Stage
    {
            const CSMWorld::IdCollection<ESM::BirthSign> &mBirthsigns;
            const CSMWorld::Resources &mTextures;
            bool mIgnoreBaseRecords;

        public:

            BirthsignCheckStage (const CSMWorld::IdCollection<ESM::BirthSign> &birthsigns,
                                 const CSMWorld::Resources &textures);

            int setup() override;
            ///< \return number of steps

            void perform (int stage, CSMDoc::Messages& messages) override;
            ///< Messages resulting from this tage will be appended to \a messages.
    };
}

#endif
