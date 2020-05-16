#ifndef GAME_MWMECHANICS_AIACTIVATE_H
#define GAME_MWMECHANICS_AIACTIVATE_H

#include "typedaipackage.hpp"

#include <string>

#include "pathfinding.hpp"

namespace ESM
{
namespace AiSequence
{
    struct AiActivate;
}
}

namespace MWMechanics
{
    /// \brief Causes actor to walk to activatable object and activate it
    /** Will activate when close to object **/
    class AiActivate final : public TypedAiPackage<AiActivate>
    {
        public:
            /// Constructor
            /** \param objectId Reference to object to activate **/
            AiActivate(const std::string &objectId);

            AiActivate(const ESM::AiSequence::AiActivate* activate);

            bool execute (const MWWorld::Ptr& actor, CharacterController& characterController, AiState& state, float duration) final;

            static constexpr AiPackageTypeId getTypeId() { return AiPackageTypeId::Activate; }

            void writeState(ESM::AiSequence::AiSequence& sequence) const final;

        private:
            const std::string mObjectId;
    };
}
#endif // GAME_MWMECHANICS_AIACTIVATE_H
