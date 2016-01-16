#ifndef CSM_TOOLS_GMSTCHECK_H
#define CSM_TOOLS_GMSTCHECK_H

#include <components/esm/loadgmst.hpp>

#include "../world/idcollection.hpp"

#include "../doc/stage.hpp"

namespace CSMTools
{
    /// \brief VerifyStage: make sure that GMSTs are alright
    class GMSTCheckStage : public CSMDoc::Stage
    {
    public:
        
        GMSTCheckStage(const CSMWorld::IdCollection<ESM::GameSetting>& gameSettings);
        
        virtual int setup();
        ///< \return number of steps

        virtual void perform(int stage, CSMDoc::Messages& messages);
        ///< Messages resulting from this stage will be appended to \a messages
        
    private:
        
        const CSMWorld::IdCollection<ESM::GameSetting>& mGameSettings;
        
    };
}

#endif
