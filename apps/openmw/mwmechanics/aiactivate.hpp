#ifndef GAME_MWMECHANICS_AIACTIVATE_H
#define GAME_MWMECHANICS_AIACTIVATE_H

#include "aipackage.hpp"


namespace MWMechanics
{

class AiActivate : AiPackage
{
    public:
    AiActivate(const MWWorld::Ptr& object);
    virtual AiActivate *clone() const;
    virtual bool execute (const MWWorld::Ptr& actor);
            ///< \return Package completed?
    virtual int getTypeId() const;

    private:
    const MWWorld::Ptr * mObject;
};
}
#endif // GAME_MWMECHANICS_AIACTIVATE_H
