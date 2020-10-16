#ifndef CSM_TOOLS_SOUNDGENCHECK_HPP
#define CSM_TOOLS_SOUNDGENCHECK_HPP

#include "../world/data.hpp"

#include "../doc/stage.hpp"

namespace CSMTools
{
    /// \brief VerifyStage: make sure that sound gen records are internally consistent
    class SoundGenCheckStage : public CSMDoc::Stage
    {
            const CSMWorld::IdCollection<ESM::SoundGenerator> &mSoundGens;
            const CSMWorld::IdCollection<ESM::Sound> &mSounds;
            const CSMWorld::RefIdCollection &mObjects;
            bool mIgnoreBaseRecords;

        public:
            SoundGenCheckStage(const CSMWorld::IdCollection<ESM::SoundGenerator> &soundGens,
                               const CSMWorld::IdCollection<ESM::Sound> &sounds,
                               const CSMWorld::RefIdCollection &objects);

            int setup() override;
            ///< \return number of steps

            void perform(int stage, CSMDoc::Messages &messages) override;
            ///< Messages resulting from this stage will be appended to \a messages.
    };
}

#endif
