#ifndef GAME_MWMECHANICS_AIACTIVATE_H
#define GAME_MWMECHANICS_AIACTIVATE_H

#include "aipackage.hpp"
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
    class AiActivate : public AiPackage
    {
        public:
            /// Constructor
            /** \param objectId Reference to object to activate **/
            AiActivate(const std::string &objectId);

            AiActivate(const ESM::AiSequence::AiActivate* activate);

            virtual AiActivate *clone() const;
            virtual bool execute (const MWWorld::Ptr& actor, CharacterController& characterController, AiState& state, float duration);
            virtual int getTypeId() const;

            virtual void writeState(ESM::AiSequence::AiSequence& sequence) const;

        private:
            std::string mObjectId;
    };
}
#endif // GAME_MWMECHANICS_AIACTIVATE_H
