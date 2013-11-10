#include <boost/algorithm/string.hpp>

#include <OgreMath.h>
#include <OgreSceneNode.h>

#include "../mwworld/esmstore.hpp"
#include <components/esm/loadcell.hpp>

#include <components/compiler/extensions.hpp>
#include <components/compiler/opcodes.hpp>

#include <components/interpreter/interpreter.hpp>
#include <components/interpreter/runtime.hpp>
#include <components/interpreter/opcodes.hpp>

#include "../mwbase/environment.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/player.hpp"
#include "../mwworld/manualref.hpp"

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
                    runtime.push(ptr.getCellRef().mScale);
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
                    MWBase::Environment::get().getWorld()->scaleObject(ptr,ptr.getCellRef().mScale + scale);
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
                    Interpreter::Type_Float angle = runtime[0].mFloat;
                    runtime.pop();

                    float ax = Ogre::Radian(ptr.getRefData().getLocalRotation().rot[0]).valueDegrees();
                    float ay = Ogre::Radian(ptr.getRefData().getLocalRotation().rot[1]).valueDegrees();
                    float az = Ogre::Radian(ptr.getRefData().getLocalRotation().rot[2]).valueDegrees();

                    float *objRot = ptr.getRefData().getPosition().rot;

                    float lx = Ogre::Radian(objRot[0]).valueDegrees();
                    float ly = Ogre::Radian(objRot[1]).valueDegrees();
                    float lz = Ogre::Radian(objRot[2]).valueDegrees();

                    if (axis == "x")
                    {
                        MWBase::Environment::get().getWorld()->localRotateObject(ptr,angle-lx,ay,az);
                    }
                    else if (axis == "y")
                    {
                        MWBase::Environment::get().getWorld()->localRotateObject(ptr,ax,angle-ly,az);
                    }
                    else if (axis == "z")
                    {
                        MWBase::Environment::get().getWorld()->localRotateObject(ptr,ax,ay,angle-lz);
                    }
                    else
                        throw std::runtime_error ("invalid ration axis: " + axis);
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
                        runtime.push(Ogre::Radian(ptr.getCellRef().mPos.rot[0]).valueDegrees());
                    }
                    else if (axis == "y")
                    {
                        runtime.push(Ogre::Radian(ptr.getCellRef().mPos.rot[1]).valueDegrees());
                    }
                    else if (axis == "z")
                    {
                        runtime.push(Ogre::Radian(ptr.getCellRef().mPos.rot[2]).valueDegrees());
                    }
                    else
                        throw std::runtime_error ("invalid ration axis: " + axis);
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
                        runtime.push(Ogre::Radian(ptr.getCellRef().mPos.rot[0]).valueDegrees()+Ogre::Radian(ptr.getRefData().getLocalRotation().rot[0]).valueDegrees());
                    }
                    else if (axis=="y")
                    {
                        runtime.push(Ogre::Radian(ptr.getCellRef().mPos.rot[1]).valueDegrees()+Ogre::Radian(ptr.getRefData().getLocalRotation().rot[1]).valueDegrees());
                    }
                    else if (axis=="z")
                    {
                        runtime.push(Ogre::Radian(ptr.getCellRef().mPos.rot[2]).valueDegrees()+Ogre::Radian(ptr.getRefData().getLocalRotation().rot[2]).valueDegrees());
                    }
                    else
                        throw std::runtime_error ("invalid ration axis: " + axis);
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
                        throw std::runtime_error ("invalid rotation axis: " + axis);                    
                }
        };

        template<class R>
        class OpSetPos : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    std::string axis = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();
                    Interpreter::Type_Float pos = runtime[0].mFloat;
                    runtime.pop();

                    float ax = ptr.getRefData().getPosition().pos[0];
                    float ay = ptr.getRefData().getPosition().pos[1];
                    float az = ptr.getRefData().getPosition().pos[2];

                    if(axis == "x")
                    {
                        MWBase::Environment::get().getWorld()->moveObject(ptr,pos,ay,az);
                    }
                    else if(axis == "y")
                    {
                        MWBase::Environment::get().getWorld()->moveObject(ptr,ax,pos,az);
                    }
                    else if(axis == "z")
                    {
                        MWBase::Environment::get().getWorld()->moveObject(ptr,ax,ay,pos);
                    }
                    else
                        throw std::runtime_error ("invalid axis: " + axis);
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
                        runtime.push(ptr.getCellRef().mPos.pos[0]);
                    }
                    else if(axis == "y")
                    {
                        runtime.push(ptr.getCellRef().mPos.pos[1]);
                    }
                    else if(axis == "z")
                    {
                        runtime.push(ptr.getCellRef().mPos.pos[2]);
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
                    catch(std::exception &e)
                    {
                        const ESM::Cell* cell = MWBase::Environment::get().getWorld()->getExterior(cellID);
                        if(cell)
                        {
                            int cx,cy;
                            MWBase::Environment::get().getWorld()->positionToIndex(x,y,cx,cy);
                            store = MWBase::Environment::get().getWorld()->getExterior(cx,cy);
                        }
                    }
                    if(store)
                    {
                        MWBase::Environment::get().getWorld()->moveObject(ptr,*store,x,y,z);
                        float ax = Ogre::Radian(ptr.getRefData().getPosition().rot[0]).valueDegrees();
                        float ay = Ogre::Radian(ptr.getRefData().getPosition().rot[1]).valueDegrees();
                        if(ptr.getTypeName() == typeid(ESM::NPC).name())//some morrowind oddity
                        {
                            ax = ax/60.;
                            ay = ay/60.;
                            zRot = zRot/60.;
                        }
                        MWBase::Environment::get().getWorld()->rotateObject(ptr,ax,ay,zRot);

                        MWWorld::Class::get(ptr).adjustPosition(ptr);
                    }
                    else
                    {
                        throw std::runtime_error ("unknown cell");
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
                    MWBase::Environment::get().getWorld()->moveObject(ptr,
                        *MWBase::Environment::get().getWorld()->getExterior(cx,cy),x,y,z);
                    float ax = Ogre::Radian(ptr.getRefData().getPosition().rot[0]).valueDegrees();
                    float ay = Ogre::Radian(ptr.getRefData().getPosition().rot[1]).valueDegrees();
                    if(ptr.getTypeName() == typeid(ESM::NPC).name())//some morrowind oddity
                    {
                        ax = ax/60.;
                        ay = ay/60.;
                        zRot = zRot/60.;
                    }
                    MWBase::Environment::get().getWorld()->rotateObject(ptr,ax,ay,zRot);
                    MWWorld::Class::get(ptr).adjustPosition(ptr);
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
                    Interpreter::Type_Float zRot = runtime[0].mFloat;
                    runtime.pop();

                    MWWorld::CellStore* store = 0;
                    try
                    {
                        store = MWBase::Environment::get().getWorld()->getInterior(cellID);
                    }
                    catch(std::exception &e)
                    {
                        const ESM::Cell* cell = MWBase::Environment::get().getWorld()->getExterior(cellID);
                        if(cell)
                        {
                            int cx,cy;
                            MWBase::Environment::get().getWorld()->positionToIndex(x,y,cx,cy);
                            store = MWBase::Environment::get().getWorld()->getExterior(cx,cy);
                        }
                    }
                    if(store)
                    {
                        ESM::Position pos;
                        pos.pos[0] = x;
                        pos.pos[1] = y;
                        pos.pos[2] = z;
                        pos.rot[0] = pos.rot[1] = 0;
                        pos.rot[2]  = zRot;
                        MWWorld::ManualRef ref(MWBase::Environment::get().getWorld()->getStore(),itemID);
                        ref.getPtr().getCellRef().mPos = pos;
                        MWBase::Environment::get().getWorld()->safePlaceObject(ref.getPtr(),*store,pos);
                    }
                    else
                    {
                        throw std::runtime_error ("unknown cell");
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
                    Interpreter::Type_Float zRot = runtime[0].mFloat;
                    runtime.pop();

                    int cx,cy;
                    MWBase::Environment::get().getWorld()->positionToIndex(x,y,cx,cy);
                    MWWorld::CellStore* store = MWBase::Environment::get().getWorld()->getExterior(cx,cy);
                    if(store)
                    {
                        ESM::Position pos;
                        pos.pos[0] = x;
                        pos.pos[1] = y;
                        pos.pos[2] = z;
                        pos.rot[0] = pos.rot[1] = 0;
                        pos.rot[2]  = zRot;
                        MWWorld::ManualRef ref(MWBase::Environment::get().getWorld()->getStore(),itemID);
                        ref.getPtr().getCellRef().mPos = pos;
                        MWBase::Environment::get().getWorld()->safePlaceObject(ref.getPtr(),*store,pos);
                    }
                    else
                    {
                        throw std::runtime_error ("unknown cell");
                    }
                }
        };

        template<class R, bool pc>
        class OpPlaceAt : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr actor = pc
                        ? MWBase::Environment::get().getWorld()->getPlayer().getPlayer()
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

                    // no-op
                    if (count == 0)
                        return;

                    ESM::Position ipos = actor.getRefData().getPosition();
                    Ogre::Vector3 pos(ipos.pos[0],ipos.pos[1],ipos.pos[2]);
                    Ogre::Quaternion rot(Ogre::Radian(-ipos.rot[2]), Ogre::Vector3::UNIT_Z);
                    if(direction == 0) pos = pos + distance*rot.yAxis();
                    else if(direction == 1) pos = pos - distance*rot.yAxis();
                    else if(direction == 2) pos = pos - distance*rot.xAxis();
                    else if(direction == 3) pos = pos + distance*rot.xAxis();
                    else throw std::runtime_error ("direction must be 0,1,2 or 3");

                    ipos.pos[0] = pos.x;
                    ipos.pos[1] = pos.y;
                    ipos.pos[2] = pos.z;
                    ipos.rot[0] = 0;
                    ipos.rot[1] = 0;
                    ipos.rot[2] = 0;

                    // create item
                    MWWorld::CellStore* store = actor.getCell();
                    MWWorld::ManualRef ref(MWBase::Environment::get().getWorld()->getStore(), itemID, count);
                    ref.getPtr().getCellRef().mPos = ipos;

                    MWBase::Environment::get().getWorld()->safePlaceObject(ref.getPtr(),*store,ipos);
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
                    Interpreter::Type_Float rotation = (runtime[0].mFloat*MWBase::Environment::get().getFrameDuration());
                    runtime.pop();

                    float ax = Ogre::Radian(ptr.getRefData().getLocalRotation().rot[0]).valueDegrees();
                    float ay = Ogre::Radian(ptr.getRefData().getLocalRotation().rot[1]).valueDegrees();
                    float az = Ogre::Radian(ptr.getRefData().getLocalRotation().rot[2]).valueDegrees();

                    if (axis == "x")
                    {
                        MWBase::Environment::get().getWorld()->localRotateObject(ptr,ax+rotation,ay,az);
                    }
                    else if (axis == "y")
                    {
                        MWBase::Environment::get().getWorld()->localRotateObject(ptr,ax,ay+rotation,az);
                    }
                    else if (axis == "z")
                    {
                        MWBase::Environment::get().getWorld()->localRotateObject(ptr,ax,ay,az+rotation);
                    }
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
                    Interpreter::Type_Float rotation = (runtime[0].mFloat*MWBase::Environment::get().getFrameDuration());
                    runtime.pop();

                    float *objRot = ptr.getRefData().getPosition().rot;

                    float ax = Ogre::Radian(objRot[0]).valueDegrees();
                    float ay = Ogre::Radian(objRot[1]).valueDegrees();
                    float az = Ogre::Radian(objRot[2]).valueDegrees();

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
                    ptr.getRefData().getLocalRotation().rot[0] = 0;
                    ptr.getRefData().getLocalRotation().rot[1] = 0;
                    ptr.getRefData().getLocalRotation().rot[2] = 0;
                    MWBase::Environment::get().getWorld()->rotateObject(ptr, 0,0,0,true);
                    MWBase::Environment::get().getWorld()->moveObject(ptr, ptr.getCellRef().mPos.pos[0],
                            ptr.getCellRef().mPos.pos[1], ptr.getCellRef().mPos.pos[2]);

                }
        };

        template<class R>
        class OpMove : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    const MWWorld::Ptr& ptr = R()(runtime);

                    std::string axis = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();
                    Interpreter::Type_Float movement = (runtime[0].mFloat*MWBase::Environment::get().getFrameDuration());
                    runtime.pop();

                    Ogre::Vector3 posChange;
                    if (axis == "x")
                    {
                        posChange=Ogre::Vector3(movement, 0, 0);
                    }
                    else if (axis == "y")
                    {
                        posChange=Ogre::Vector3(0, movement, 0);
                    }
                    else if (axis == "z")
                    {
                        posChange=Ogre::Vector3(0, 0, movement);
                    }
                    else
                        throw std::runtime_error ("invalid movement axis: " + axis);

                    Ogre::Vector3 worldPos = ptr.getRefData().getBaseNode()->convertLocalToWorldPosition(posChange);
                    MWBase::Environment::get().getWorld()->moveObject(ptr, worldPos.x, worldPos.y, worldPos.z);
                }
        };

        template<class R>
        class OpMoveWorld : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    std::string axis = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();
                    Interpreter::Type_Float movement = (runtime[0].mFloat*MWBase::Environment::get().getFrameDuration());
                    runtime.pop();

                    float *objPos = ptr.getRefData().getPosition().pos;

                    if (axis == "x")
                    {
                        MWBase::Environment::get().getWorld()->moveObject(ptr, objPos[0]+movement, objPos[1], objPos[2]);
                    }
                    else if (axis == "y")
                    {
                        MWBase::Environment::get().getWorld()->moveObject(ptr, objPos[0], objPos[1]+movement, objPos[2]);
                    }
                    else if (axis == "z")
                    {
                        MWBase::Environment::get().getWorld()->moveObject(ptr, objPos[0], objPos[1], objPos[2]+movement);
                    }
                    else
                        throw std::runtime_error ("invalid movement axis: " + axis);
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
        }
    }
}
