#ifndef GAME_MWMECHANICS_AIACTIVATE_H
#define GAME_MWMECHANICS_AIACTIVATE_H

#include "aipackage.hpp"
#include <string>

#include "pathfinding.hpp"

namespace MWMechanics
{
    /// \brief Causes actor to walk to activatable object and activate it
    /** Will actiavte when close to object or path grid complete **/
    class AiActivate : public AiPackage
    {
        public:
            /// Constructor
            /** \param objectId Reference to object to activate **/
            AiActivate(const std::string &objectId);
            virtual AiActivate *clone() const;
            virtual bool execute (const MWWorld::Ptr& actor,float duration);
            virtual int getTypeId() const;

        private:
            std::string mObjectId;
            int mCellX;
            int mCellY;
    };
}
#endif // GAME_MWMECHANICS_AIACTIVATE_H
