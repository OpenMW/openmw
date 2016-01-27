#ifndef CSM_TOOLS_JOURNALINFOCHECK_H
#define CSM_TOOLS_JOURNALINFOCHECK_H

#include <components/esm/loaddial.hpp>

#include "../world/idcollection.hpp"
#include "../world/infocollection.hpp"

#include "../doc/stage.hpp"

namespace CSMTools
{
    /// \brief VerifyStage: make sure that journal infos are good
    class JournalInfoCheckStage : public CSMDoc::Stage
    {
    public:
        
        JournalInfoCheckStage(const CSMWorld::IdCollection<ESM::Dialogue>& journals,
                              const CSMWorld::InfoCollection& journalInfos);
        
        virtual int setup();
        ///< \return number of steps

        virtual void perform(int stage, CSMDoc::Messages& messages);
        ///< Messages resulting from this stage will be appended to \a messages
        
    private:
        
        const CSMWorld::IdCollection<ESM::Dialogue>& mJournals;
        const CSMWorld::InfoCollection& mJournalInfos;
        
    };
}

#endif
