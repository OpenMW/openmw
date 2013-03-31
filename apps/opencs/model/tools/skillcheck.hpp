#ifndef CSM_TOOLS_SKILLCHECK_H
#define CSM_TOOLS_SKILLCHECK_H

#include <components/esm/loadskil.hpp>

#include "../world/idcollection.hpp"

#include "stage.hpp"

namespace CSMTools
{
    /// \brief VerifyStage: make sure that skill records are internally consistent
    class SkillCheckStage : public Stage
    {
            const CSMWorld::IdCollection<ESM::Skill>& mSkills;

        public:

            SkillCheckStage (const CSMWorld::IdCollection<ESM::Skill>& skills);

            virtual int setup();
            ///< \return number of steps

            virtual void perform (int stage, std::vector<std::string>& messages);
            ///< Messages resulting from this tage will be appended to \a messages.
    };
}

#endif
