#ifndef CSM_TOOLS_SKILLCHECK_H
#define CSM_TOOLS_SKILLCHECK_H

#include <components/esm/loadskil.hpp>

#include "../world/idcollection.hpp"

#include "../doc/stage.hpp"

namespace CSMTools
{
    /// \brief VerifyStage: make sure that skill records are internally consistent
    class SkillCheckStage : public CSMDoc::Stage
    {
            const CSMWorld::IdCollection<ESM::Skill>& mSkills;
            bool mIgnoreBaseRecords;

        public:

            SkillCheckStage (const CSMWorld::IdCollection<ESM::Skill>& skills);

            virtual int setup();
            ///< \return number of steps

            virtual void perform (int stage, CSMDoc::Messages& messages);
            ///< Messages resulting from this tage will be appended to \a messages.
    };
}

#endif
