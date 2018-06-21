#ifndef CSM_TOOLS_MAGICEFFECTCHECK_HPP
#define CSM_TOOLS_MAGICEFFECTCHECK_HPP

#include <components/esm/loadmgef.hpp>
#include <components/esm/loadsoun.hpp>

#include "../world/idcollection.hpp"
#include "../world/refidcollection.hpp"

#include "../doc/stage.hpp"

namespace CSMWorld
{
    class Resources;
}

namespace CSMTools
{
    /// \brief VerifyStage: make sure that magic effect records are internally consistent
    class MagicEffectCheckStage : public CSMDoc::Stage
    {
            const CSMWorld::IdCollection<ESM::MagicEffect> &mMagicEffects;
            const CSMWorld::IdCollection<ESM::Sound> &mSounds;
            const CSMWorld::RefIdCollection &mReferenceables;
            const CSMWorld::Resources &mIcons;
            const CSMWorld::Resources &mTextures;
            bool mIgnoreBaseRecords;

        private:
            bool isTextureExists(const std::string &texture, bool isIcon) const;

            std::string checkReferenceable(const std::string &id,
                                           const CSMWorld::UniversalId &type,
                                           const std::string &column) const;
            std::string checkSound(const std::string &id, const std::string &column) const;

        public:
            MagicEffectCheckStage(const CSMWorld::IdCollection<ESM::MagicEffect> &effects,
                                  const CSMWorld::IdCollection<ESM::Sound> &sounds,
                                  const CSMWorld::RefIdCollection &referenceables,
                                  const CSMWorld::Resources &icons,
                                  const CSMWorld::Resources &textures);

            virtual int setup();
            ///< \return number of steps
            virtual void perform (int stage, CSMDoc::Messages &messages);
            ///< Messages resulting from this tage will be appended to \a messages.
    };
}

#endif
