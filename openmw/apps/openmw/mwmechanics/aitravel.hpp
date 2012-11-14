#ifndef AITRAVEL_HPP_INCLUDED
#define AITRAVEL_HPP_INCLUDED

#include "aipackage.hpp"
#include <iostream>

namespace MWWorld
{
class Ptr;
}

namespace MWMechanics
{
class AiTravel : public AiPackage
{
    public:
    AiTravel(float x, float y, float z, bool reset = false);
    virtual AiTravel *clone() const;

    virtual bool execute (const MWWorld::Ptr& actor);
            ///< \return Package completed?

    virtual int getTypeId() const;
    float getX();
    float getY();
    float getZ();
    bool getReset();

    private:
    float mX;
    float mY;
    float mZ;
    bool mReset;
};
}

#endif // AITRAVEL_HPP_INCLUDED
