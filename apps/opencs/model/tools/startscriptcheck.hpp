#ifndef CSM_TOOLS_STARTSCRIPTCHECK_H
#define CSM_TOOLS_STARTSCRIPTCHECK_H

#include <components/esm3/loadsscr.hpp>
#include <components/esm3/loadscpt.hpp>

#include "../doc/stage.hpp"

#include "../world/idcollection.hpp"

namespace CSMTools
{
    class StartScriptCheckStage : public CSMDoc::Stage
    {
            const CSMWorld::IdCollection<ESM::StartScript>& mStartScripts;
            const CSMWorld::IdCollection<ESM::Script>& mScripts;
            bool mIgnoreBaseRecords;

        public:

            StartScriptCheckStage (const CSMWorld::IdCollection<ESM::StartScript>& startScripts,
                const CSMWorld::IdCollection<ESM::Script>& scripts);

            void perform(int stage, CSMDoc::Messages& messages) override;
            int setup() override;
    };
}

#endif
