#include <iostream>

#include <osg/PositionAttitudeTransform>

#include <components/esm/loadcell.hpp>

#include <components/compiler/extensions.hpp>
#include <components/compiler/opcodes.hpp>

#include <components/interpreter/interpreter.hpp>
#include <components/interpreter/runtime.hpp>
#include <components/interpreter/opcodes.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/cellstore.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/manualref.hpp"
#include "../mwworld/player.hpp"
#include "../mwworld/esmstore.hpp"

#include "../mwmechanics/actorutil.hpp"

#include "interpretercontext.hpp"
#include "ref.hpp"

namespace MWScript
{
    namespace Transformation
    {
        template<class R>
        class OpSetScale : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    Interpreter::Type_Float scale = runtime[0].mFloat;
                    runtime.pop();

                    MWBase::Environment::get().getWorld()->scaleObject(ptr,scale);
                }
        };

        template<class R>
        class OpGetScale : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);
                    runtime.push(ptr.getCellRef().getScale());
                }
        };

        template<class R>
        class OpModScale : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    Interpreter::Type_Float scale = runtime[0].mFloat;
                    runtime.pop();

                    // add the parameter to the object's scale.
                    MWBase::Environment::get().getWorld()->scaleObject(ptr,ptr.getCellRef().getScale() + scale);
                }
        };

        template<class R>
        class OpSetAngle : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    std::string axis = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();
                    Interpreter::Type_Float angle = osg::DegreesToRadians(runtime[0].mFloat);
                    runtime.pop();

                    float ax = ptr.getRefData().getPosition().rot[0];
                    float ay = ptr.getRefData().getPosition().rot[1];
                    float az = ptr.getRefData().getPosition().rot[2];

                    if (axis == "x")
                        MWBase::Environment::get().getWorld()->rotateObject(ptr,angle,ay,az);
                    else if (axis == "y")
                        MWBase::Environment::get().getWorld()->rotateObject(ptr,ax,angle,az);
                    else if (axis == "z")
                        MWBase::Environment::get().getWorld()->rotateObject(ptr,ax,ay,angle);
                    else
                        throw std::runtime_error ("invalid rotation axis: " + axis);
                }
        };

        template<class R>
        class OpGetStartingAngle : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    std::string axis = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    if (axis == "x")
                    {
                        runtime.push(osg::RadiansToDegrees(ptr.getCellRef().getPosition().rot[0]));
                    }
                    else if (axis == "y")
                    {
                        runtime.push(osg::RadiansToDegrees(ptr.getCellRef().getPosition().rot[1]));
                    }
                    else if (axis == "z")
                    {
                        runtime.push(osg::RadiansToDegrees(ptr.getCellRef().getPosition().rot[2]));
                    }
                    else
                        throw std::runtime_error ("invalid rotation axis: " + axis);
                }
        };

        template<class R>
        class OpGetAngle : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    std::string axis = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    if (axis=="x")
                    {
                        runtime.push(osg::RadiansToDegrees(ptr.getRefData().getPosition().rot[0]));
                    }
                    else if (axis=="y")
                    {
                        runtime.push(osg::RadiansToDegrees(ptr.getRefData().getPosition().rot[1]));
                    }
                    else if (axis=="z")
                    {
                        runtime.push(osg::RadiansToDegrees(ptr.getRefData().getPosition().rot[2]));
                    }
                    else
                        throw std::runtime_error ("invalid rotation axis: " + axis);
                }
        };

        template<class R>
        class OpGetPos : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    std::string axis = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    if(axis == "x")
                    {
                        runtime.push(ptr.getRefData().getPosition().pos[0]);
                    }
                    else if(axis == "y")
                    {
                        runtime.push(ptr.getRefData().getPosition().pos[1]);
                    }
                    else if(axis == "z")
                    {
                        runtime.push(ptr.getRefData().getPosition().pos[2]);
                    }
                    else
                        throw std::runtime_error ("invalid axis: " + axis);
                }
        };

        template<class R>
        class OpSetPos : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    if (!ptr.isInCell())
                        return;

                    if (ptr == MWMechanics::getPlayer())
                    {
                        MWBase::Environment::get().getWorld()->getPlayer().setTeleported(true);
                    }

                    std::string axis = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();
                    Interpreter::Type_Float pos = runtime[0].mFloat;
                    runtime.pop();

                    float ax = ptr.getRefData().getPosition().pos[0];
                    float ay = ptr.getRefData().getPosition().pos[1];
                    float az = ptr.getRefData().getPosition().pos[2];

                    MWWorld::Ptr updated = ptr;
                    if(axis == "x")
                    {
                        updated = MWBase::Environment::get().getWorld()->moveObject(ptr,pos,ay,az);
                    }
                    else if(axis == "y")
                    {
                        updated = MWBase::Environment::get().getWorld()->moveObject(ptr,ax,pos,az);
                    }
                    else if(axis == "z")
                    {
                        updated = MWBase::Environment::get().getWorld()->moveObject(ptr,ax,ay,pos);
                    }
                    else
                        throw std::runtime_error ("invalid axis: " + axis);

                    dynamic_cast<MWScript::InterpreterContext&>(runtime.getContext()).updatePtr(ptr,updated);
                }
        };

        template<class R>
        class OpGetStartingPos : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    std::string axis = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    if(axis == "x")
                    {
                        runtime.push(ptr.getCellRef().getPosition().pos[0]);
                    }
                    else if(axis == "y")
                    {
                        runtime.push(ptr.getCellRef().getPosition().pos[1]);
                    }
                    else if(axis == "z")
                    {
                        runtime.push(ptr.getCellRef().getPosition().pos[2]);
                    }
                    else
                        throw std::runtime_error ("invalid axis: " + axis);
                }
        };

        template<class R>
        class OpPositionCell : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    if (ptr.getContainerStore())
                        return;

                    if (ptr == MWMechanics::getPlayer())
                    {
                        MWBase::Environment::get().getWorld()->getPlayer().setTeleported(true);
                    }

                    Interpreter::Type_Float x = runtime[0].mFloat;
                    runtime.pop();
                    Interpreter::Type_Float y = runtime[0].mFloat;
                    runtime.pop();
                    Interpreter::Type_Float z = runtime[0].mFloat;
                    runtime.pop();
                    Interpreter::Type_Float zRot = runtime[0].mFloat;
                    runtime.pop();
                    std::string cellID = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    MWWorld::CellStore* store = 0;
                    try
                    {
                        store = MWBase::Environment::get().getWorld()->getInterior(cellID);
                    }
                    catch(std::exception&)
                    {
                        const ESM::Cell* cell = MWBase::Environment::get().getWorld()->getExterior(cellID);
                        int cx,cy;
                        MWBase::Environment::get().getWorld()->positionToIndex(x,y,cx,cy);
                        store = MWBase::Environment::get().getWorld()->getExterior(cx,cy);
                        if(!cell)
                        {
                            runtime.getContext().report ("unknown cell (" + cellID + ")");
                            std::cerr << "unknown cell (" << cellID << ")\n";
                        }
                    }
                    if(store)
                    {
                        MWWorld::Ptr base = ptr;
                        ptr = MWBase::Environment::get().getWorld()->moveObject(ptr,store,x,y,z);
                        dynamic_cast<MWScript::InterpreterContext&>(runtime.getContext()).updatePtr(base,ptr);

                        float ax = osg::RadiansToDegrees(ptr.getRefData().getPosition().rot[0]);
                        float ay = osg::RadiansToDegrees(ptr.getRefData().getPosition().rot[1]);
                        // Note that you must specify ZRot in minutes (1 degree = 60 minutes; north = 0, east = 5400, south = 10800, west = 16200)
                        // except for when you position the player, then degrees must be used.
                        // See "Morrowind Scripting for Dummies (9th Edition)" pages 50 and 54 for reference.
                        if(ptr != MWMechanics::getPlayer())
                            zRot = zRot/60.0f;
                        MWBase::Environment::get().getWorld()->rotateObject(ptr,ax,ay,osg::DegreesToRadians(zRot));

                        ptr.getClass().adjustPosition(ptr, false);
                    }
                }
        };

        template<class R>
        class OpPosition : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    if (!ptr.isInCell())
                        return;

                    if (ptr == MWMechanics::getPlayer())
                    {
                        MWBase::Environment::get().getWorld()->getPlayer().setTeleported(true);
                    }

                    Interpreter::Type_Float x = runtime[0].mFloat;
                    runtime.pop();
                    Interpreter::Type_Float y = runtime[0].mFloat;
                    runtime.pop();
                    Interpreter::Type_Float z = runtime[0].mFloat;
                    runtime.pop();
                    Interpreter::Type_Float zRot = runtime[0].mFloat;
                    runtime.pop();
                    int cx,cy;
                    MWBase::Environment::get().getWorld()->positionToIndex(x,y,cx,cy);

                    // another morrowind oddity: player will be moved to the exterior cell at this location,
                    // non-player actors will move within the cell they are in.
                    MWWorld::Ptr base = ptr;
                    if (ptr == MWMechanics::getPlayer())
                    {
                        MWWorld::CellStore* cell = MWBase::Environment::get().getWorld()->getExterior(cx,cy);
                        ptr = MWBase::Environment::get().getWorld()->moveObject(ptr,cell,x,y,z);
                    }
                    else
                    {
                        ptr = MWBase::Environment::get().getWorld()->moveObject(ptr, x, y, z);
                    }
                    dynamic_cast<MWScript::InterpreterContext&>(runtime.getContext()).updatePtr(base,ptr);

                    float ax = osg::RadiansToDegrees(ptr.getRefData().getPosition().rot[0]);
                    float ay = osg::RadiansToDegrees(ptr.getRefData().getPosition().rot[1]);
                    // Note that you must specify ZRot in minutes (1 degree = 60 minutes; north = 0, east = 5400, south = 10800, west = 16200)
                    // except for when you position the player, then degrees must be used.
                    // See "Morrowind Scripting for Dummies (9th Edition)" pages 50 and 54 for reference.
                    if(ptr != MWMechanics::getPlayer())
                        zRot = zRot/60.0f;
                    MWBase::Environment::get().getWorld()->rotateObject(ptr,ax,ay,zRot);
                    ptr.getClass().adjustPosition(ptr, false);
                }
        };

        template<class R>
        class OpPlaceItemCell : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    std::string itemID = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();
                    std::string cellID = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    Interpreter::Type_Float x = runtime[0].mFloat;
                    runtime.pop();
                    Interpreter::Type_Float y = runtime[0].mFloat;
                    runtime.pop();
                    Interpreter::Type_Float z = runtime[0].mFloat;
                    runtime.pop();
                    Interpreter::Type_Float zRotDegrees = runtime[0].mFloat;
                    runtime.pop();

                    MWWorld::CellStore* store = 0;
                    try
                    {
                        store = MWBase::Environment::get().getWorld()->getInterior(cellID);
                    }
                    catch(std::exception&)
                    {
                        const ESM::Cell* cell = MWBase::Environment::get().getWorld()->getExterior(cellID);
                        int cx,cy;
                        MWBase::Environment::get().getWorld()->positionToIndex(x,y,cx,cy);
                        store = MWBase::Environment::get().getWorld()->getExterior(cx,cy);
                        if(!cell)
                        {
                            runtime.getContext().report ("unknown cell (" + cellID + ")");
                            std::cerr << "unknown cell (" << cellID << ")\n";
                        }
                    }
                    if(store)
                    {
                        ESM::Position pos;
                        pos.pos[0] = x;
                        pos.pos[1] = y;
                        pos.pos[2] = z;
                        pos.rot[0] = pos.rot[1] = 0;
                        pos.rot[2] = osg::DegreesToRadians(zRotDegrees);
                        MWWorld::ManualRef ref(MWBase::Environment::get().getWorld()->getStore(),itemID);
                        ref.getPtr().getCellRef().setPosition(pos);
                        MWWorld::Ptr placed = MWBase::Environment::get().getWorld()->safePlaceObject(ref.getPtr(),store,pos);
                        placed.getClass().adjustPosition(placed, true);
                    }
                }
        };

        template<class R>
        class OpPlaceItem : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    std::string itemID = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    Interpreter::Type_Float x = runtime[0].mFloat;
                    runtime.pop();
                    Interpreter::Type_Float y = runtime[0].mFloat;
                    runtime.pop();
                    Interpreter::Type_Float z = runtime[0].mFloat;
                    runtime.pop();
                    Interpreter::Type_Float zRotDegrees = runtime[0].mFloat;
                    runtime.pop();

                    MWWorld::Ptr player = MWMechanics::getPlayer();
                    MWWorld::CellStore* store = NULL;
                    if (player.getCell()->isExterior())
                    {
                        int cx,cy;
                        MWBase::Environment::get().getWorld()->positionToIndex(x,y,cx,cy);
                        store = MWBase::Environment::get().getWorld()->getExterior(cx,cy);
                    }
                    else
                        store = player.getCell();

                    ESM::Position pos;
                    pos.pos[0] = x;
                    pos.pos[1] = y;
                    pos.pos[2] = z;
                    pos.rot[0] = pos.rot[1] = 0;
                    pos.rot[2] = osg::DegreesToRadians(zRotDegrees);
                    MWWorld::ManualRef ref(MWBase::Environment::get().getWorld()->getStore(),itemID);
                    ref.getPtr().getCellRef().setPosition(pos);
                    MWWorld::Ptr placed = MWBase::Environment::get().getWorld()->safePlaceObject(ref.getPtr(),store,pos);
                    placed.getClass().adjustPosition(placed, true);
                }
        };

        template<class R, bool pc>
        class OpPlaceAt : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr actor = pc
                        ? MWMechanics::getPlayer()
                        : R()(runtime);

                    std::string itemID = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    Interpreter::Type_Integer count = runtime[0].mInteger;
                    runtime.pop();
                    Interpreter::Type_Float distance = runtime[0].mFloat;
                    runtime.pop();
                    Interpreter::Type_Integer direction = runtime[0].mInteger;
                    runtime.pop();

                    if (count<0)
                        throw std::runtime_error ("count must be non-negative");

                    for (int i=0; i<count; ++i)
                    {
                        ESM::Position ipos = actor.getRefData().getPosition();
                        osg::Vec3f pos(ipos.asVec3());
                        osg::Quat rot(ipos.rot[2], osg::Vec3f(0,0,-1));
                        if(direction == 0) pos = pos + (rot * osg::Vec3f(0,1,0)) * distance;
                        else if(direction == 1) pos = pos - (rot * osg::Vec3f(0,1,0)) * distance;
                        else if(direction == 2) pos = pos - (rot * osg::Vec3f(1,0,0)) * distance;
                        else if(direction == 3) pos = pos + (rot * osg::Vec3f(1,0,0)) * distance;
                        else throw std::runtime_error ("direction must be 0,1,2 or 3");

                        ipos.pos[0] = pos.x();
                        ipos.pos[1] = pos.y();
                        ipos.pos[2] = pos.z();

                        if (actor.getClass().isActor())
                        {
                            // TODO: should this depend on the 'direction' parameter?
                            ipos.rot[0] = 0;
                            ipos.rot[1] = 0;
                            ipos.rot[2] = 0;
                        }
                        else
                        {
                            ipos.rot[0] = actor.getRefData().getPosition().rot[0];
                            ipos.rot[1] = actor.getRefData().getPosition().rot[1];
                            ipos.rot[2] = actor.getRefData().getPosition().rot[2];
                        }
                        // create item
                        MWWorld::CellStore* store = actor.getCell();
                        MWWorld::ManualRef ref(MWBase::Environment::get().getWorld()->getStore(), itemID, 1);
                        ref.getPtr().getCellRef().setPosition(ipos);

                        MWBase::Environment::get().getWorld()->safePlaceObject(ref.getPtr(),store,ipos);
                    }
                }
        };

        template<class R>
        class OpRotate : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    const MWWorld::Ptr& ptr = R()(runtime);

                    std::string axis = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();
                    Interpreter::Type_Float rotation = osg::DegreesToRadians(runtime[0].mFloat*MWBase::Environment::get().getFrameDuration());
                    runtime.pop();

                    float ax = ptr.getRefData().getPosition().rot[0];
                    float ay = ptr.getRefData().getPosition().rot[1];
                    float az = ptr.getRefData().getPosition().rot[2];

                    if (axis == "x")
                        MWBase::Environment::get().getWorld()->rotateObject(ptr,ax+rotation,ay,az);
                    else if (axis == "y")
                        MWBase::Environment::get().getWorld()->rotateObject(ptr,ax,ay+rotation,az);
                    else if (axis == "z")
                        MWBase::Environment::get().getWorld()->rotateObject(ptr,ax,ay,az+rotation);
                    else
                        throw std::runtime_error ("invalid rotation axis: " + axis);
                }
        };

        template<class R>
        class OpRotateWorld : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    std::string axis = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();
                    Interpreter::Type_Float rotation = osg::DegreesToRadians(runtime[0].mFloat*MWBase::Environment::get().getFrameDuration());
                    runtime.pop();

                    const float *objRot = ptr.getRefData().getPosition().rot;

                    float ax = objRot[0];
                    float ay = objRot[1];
                    float az = objRot[2];

                    if (axis == "x")
                    {
                        MWBase::Environment::get().getWorld()->rotateObject(ptr,ax+rotation,ay,az);
                    }
                    else if (axis == "y")
                    {
                        MWBase::Environment::get().getWorld()->rotateObject(ptr,ax,ay+rotation,az);
                    }
                    else if (axis == "z")
                    {
                        MWBase::Environment::get().getWorld()->rotateObject(ptr,ax,ay,az+rotation);
                    }
                    else
                        throw std::runtime_error ("invalid rotation axis: " + axis);
                }
        };

        template<class R>
        class OpSetAtStart : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    if (!ptr.isInCell())
                        return;

                    float xr = ptr.getCellRef().getPosition().rot[0];
                    float yr = ptr.getCellRef().getPosition().rot[1];
                    float zr = ptr.getCellRef().getPosition().rot[2];

                    MWBase::Environment::get().getWorld()->rotateObject(ptr, xr, yr, zr);

                    dynamic_cast<MWScript::InterpreterContext&>(runtime.getContext()).updatePtr(ptr,
                        MWBase::Environment::get().getWorld()->moveObject(ptr, ptr.getCellRef().getPosition().pos[0],
                            ptr.getCellRef().getPosition().pos[1], ptr.getCellRef().getPosition().pos[2]));

                }
        };

        template<class R>
        class OpMove : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    const MWWorld::Ptr& ptr = R()(runtime);

                    if (!ptr.isInCell())
                        return;

                    std::string axis = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();
                    Interpreter::Type_Float movement = (runtime[0].mFloat*MWBase::Environment::get().getFrameDuration());
                    runtime.pop();

                    osg::Vec3f posChange;
                    if (axis == "x")
                    {
                        posChange=osg::Vec3f(movement, 0, 0);
                    }
                    else if (axis == "y")
                    {
                        posChange=osg::Vec3f(0, movement, 0);
                    }
                    else if (axis == "z")
                    {
                        posChange=osg::Vec3f(0, 0, movement);
                    }
                    else
                        throw std::runtime_error ("invalid movement axis: " + axis);

                    // is it correct that disabled objects can't be Move-d?
                    if (!ptr.getRefData().getBaseNode())
                        return;

                    osg::Vec3f diff = ptr.getRefData().getBaseNode()->getAttitude() * posChange;
                    osg::Vec3f worldPos(ptr.getRefData().getPosition().asVec3());
                    worldPos += diff;
                    MWBase::Environment::get().getWorld()->moveObject(ptr, worldPos.x(), worldPos.y(), worldPos.z());
                }
        };

        template<class R>
        class OpMoveWorld : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    if (!ptr.isInCell())
                        return;

                    std::string axis = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();
                    Interpreter::Type_Float movement = (runtime[0].mFloat*MWBase::Environment::get().getFrameDuration());
                    runtime.pop();

                    const float *objPos = ptr.getRefData().getPosition().pos;

                    MWWorld::Ptr updated;
                    if (axis == "x")
                    {
                        updated = MWBase::Environment::get().getWorld()->moveObject(ptr, objPos[0]+movement, objPos[1], objPos[2]);
                    }
                    else if (axis == "y")
                    {
                        updated = MWBase::Environment::get().getWorld()->moveObject(ptr, objPos[0], objPos[1]+movement, objPos[2]);
                    }
                    else if (axis == "z")
                    {
                        updated = MWBase::Environment::get().getWorld()->moveObject(ptr, objPos[0], objPos[1], objPos[2]+movement);
                    }
                    else
                        throw std::runtime_error ("invalid movement axis: " + axis);
                }
        };

        class OpResetActors : public Interpreter::Opcode0
        {
        public:

            virtual void execute (Interpreter::Runtime& runtime)
            {
                MWBase::Environment::get().getWorld()->resetActors();
            }
        };

        void installOpcodes (Interpreter::Interpreter& interpreter)
        {
            interpreter.installSegment5(Compiler::Transformation::opcodeSetScale,new OpSetScale<ImplicitRef>);
            interpreter.installSegment5(Compiler::Transformation::opcodeSetScaleExplicit,new OpSetScale<ExplicitRef>);
            interpreter.installSegment5(Compiler::Transformation::opcodeSetAngle,new OpSetAngle<ImplicitRef>);
            interpreter.installSegment5(Compiler::Transformation::opcodeSetAngleExplicit,new OpSetAngle<ExplicitRef>);
            interpreter.installSegment5(Compiler::Transformation::opcodeGetScale,new OpGetScale<ImplicitRef>);
            interpreter.installSegment5(Compiler::Transformation::opcodeGetScaleExplicit,new OpGetScale<ExplicitRef>);
            interpreter.installSegment5(Compiler::Transformation::opcodeGetAngle,new OpGetAngle<ImplicitRef>);
            interpreter.installSegment5(Compiler::Transformation::opcodeGetAngleExplicit,new OpGetAngle<ExplicitRef>);
            interpreter.installSegment5(Compiler::Transformation::opcodeGetPos,new OpGetPos<ImplicitRef>);
            interpreter.installSegment5(Compiler::Transformation::opcodeGetPosExplicit,new OpGetPos<ExplicitRef>);
            interpreter.installSegment5(Compiler::Transformation::opcodeSetPos,new OpSetPos<ImplicitRef>);
            interpreter.installSegment5(Compiler::Transformation::opcodeSetPosExplicit,new OpSetPos<ExplicitRef>);
            interpreter.installSegment5(Compiler::Transformation::opcodeGetStartingPos,new OpGetStartingPos<ImplicitRef>);
            interpreter.installSegment5(Compiler::Transformation::opcodeGetStartingPosExplicit,new OpGetStartingPos<ExplicitRef>);
            interpreter.installSegment5(Compiler::Transformation::opcodePosition,new OpPosition<ImplicitRef>);
            interpreter.installSegment5(Compiler::Transformation::opcodePositionExplicit,new OpPosition<ExplicitRef>);
            interpreter.installSegment5(Compiler::Transformation::opcodePositionCell,new OpPositionCell<ImplicitRef>);
            interpreter.installSegment5(Compiler::Transformation::opcodePositionCellExplicit,new OpPositionCell<ExplicitRef>);
            interpreter.installSegment5(Compiler::Transformation::opcodePlaceItemCell,new OpPlaceItemCell<ImplicitRef>);
            interpreter.installSegment5(Compiler::Transformation::opcodePlaceItem,new OpPlaceItem<ImplicitRef>);
            interpreter.installSegment5(Compiler::Transformation::opcodePlaceAtPc,new OpPlaceAt<ImplicitRef, true>);
            interpreter.installSegment5(Compiler::Transformation::opcodePlaceAtMe,new OpPlaceAt<ImplicitRef, false>);
            interpreter.installSegment5(Compiler::Transformation::opcodePlaceAtMeExplicit,new OpPlaceAt<ExplicitRef, false>);
            interpreter.installSegment5(Compiler::Transformation::opcodeModScale,new OpModScale<ImplicitRef>);
            interpreter.installSegment5(Compiler::Transformation::opcodeModScaleExplicit,new OpModScale<ExplicitRef>);
            interpreter.installSegment5(Compiler::Transformation::opcodeRotate,new OpRotate<ImplicitRef>);
            interpreter.installSegment5(Compiler::Transformation::opcodeRotateExplicit,new OpRotate<ExplicitRef>);
            interpreter.installSegment5(Compiler::Transformation::opcodeRotateWorld,new OpRotateWorld<ImplicitRef>);
            interpreter.installSegment5(Compiler::Transformation::opcodeRotateWorldExplicit,new OpRotateWorld<ExplicitRef>);
            interpreter.installSegment5(Compiler::Transformation::opcodeSetAtStart,new OpSetAtStart<ImplicitRef>);
            interpreter.installSegment5(Compiler::Transformation::opcodeSetAtStartExplicit,new OpSetAtStart<ExplicitRef>);
            interpreter.installSegment5(Compiler::Transformation::opcodeMove,new OpMove<ImplicitRef>);
            interpreter.installSegment5(Compiler::Transformation::opcodeMoveExplicit,new OpMove<ExplicitRef>);
            interpreter.installSegment5(Compiler::Transformation::opcodeMoveWorld,new OpMoveWorld<ImplicitRef>);
            interpreter.installSegment5(Compiler::Transformation::opcodeMoveWorldExplicit,new OpMoveWorld<ExplicitRef>);
            interpreter.installSegment5(Compiler::Transformation::opcodeGetStartingAngle, new OpGetStartingAngle<ImplicitRef>);
            interpreter.installSegment5(Compiler::Transformation::opcodeGetStartingAngleExplicit, new OpGetStartingAngle<ExplicitRef>);
            interpreter.installSegment5(Compiler::Transformation::opcodeResetActors, new OpResetActors);
        }
    }
}
