#ifndef CSM_TOOLS_CLASSCHECK_H
#define CSM_TOOLS_CLASSCHECK_H

#include <components/esm/loadclas.hpp>

#include "../world/idcollection.hpp"

#include "../doc/stage.hpp"

namespace CSMTools
{
    /// \brief VerifyStage: make sure that class records are internally consistent
    class ClassCheckStage : public CSMDoc::Stage
    {
            const CSMWorld::IdCollection<ESM::Class>& mClasses;
            bool mIgnoreBaseRecords;

        public:

            ClassCheckStage (const CSMWorld::IdCollection<ESM::Class>& classes);

            int setup() override;
            ///< \return number of steps

            void perform (int stage, CSMDoc::Messages& messages) override;
            ///< Messages resulting from this tage will be appended to \a messages.
    };
}

#endif
