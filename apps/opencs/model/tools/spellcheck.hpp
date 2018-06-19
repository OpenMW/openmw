#ifndef CSM_TOOLS_SPELLCHECK_H
#define CSM_TOOLS_SPELLCHECK_H

#include <components/esm/loadspel.hpp>

#include "../world/idcollection.hpp"

#include "../doc/stage.hpp"

namespace CSMTools
{
    /// \brief VerifyStage: make sure that spell records are internally consistent
    class SpellCheckStage : public CSMDoc::Stage
    {
            const CSMWorld::IdCollection<ESM::Spell>& mSpells;
            bool mIgnoreBaseRecords;

        public:

            SpellCheckStage (const CSMWorld::IdCollection<ESM::Spell>& spells);

            virtual int setup();
            ///< \return number of steps

            virtual void perform (int stage, CSMDoc::Messages& messages);
            ///< Messages resulting from this tage will be appended to \a messages.
    };
}

#endif
