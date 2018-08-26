#ifndef CSM_TOOLS_SOUNDCHECK_H
#define CSM_TOOLS_SOUNDCHECK_H

#include <components/esm/loadsoun.hpp>

#include "../world/resources.hpp"
#include "../world/idcollection.hpp"

#include "../doc/stage.hpp"

namespace CSMTools
{
    /// \brief VerifyStage: make sure that sound records are internally consistent
    class SoundCheckStage : public CSMDoc::Stage
    {
            const CSMWorld::IdCollection<ESM::Sound>& mSounds;
            const CSMWorld::Resources &mSoundFiles;
            bool mIgnoreBaseRecords;

        public:

            SoundCheckStage (const CSMWorld::IdCollection<ESM::Sound>& sounds,
                             const CSMWorld::Resources &soundfiles);

            virtual int setup();
            ///< \return number of steps

            virtual void perform (int stage, CSMDoc::Messages& messages);
            ///< Messages resulting from this tage will be appended to \a messages.
    };
}

#endif
