#include <boost/algorithm/string.hpp>

#include <OgreMath.h>
#include <OgreSceneNode.h>

#include "../mwworld/esmstore.hpp"
#include <components/esm/loadcell.hpp>

#include <components/compiler/extensions.hpp>

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

                    float ax = Ogre::Radian(ptr.getRefData().getPosition().rot[0]).valueDegrees();
                    float ay = Ogre::Radian(ptr.getRefData().getPosition().rot[1]).valueDegrees();
                    float az = Ogre::Radian(ptr.getRefData().getPosition().rot[2]).valueDegrees();

                    if (axis == "x")
                    {
                        MWBase::Environment::get().getWorld()->rotateObject(ptr,angle,ay,az);
                    }
                    else if (axis == "y")
                    {
                        MWBase::Environment::get().getWorld()->rotateObject(ptr,ax,angle,az);
                    }
                    else if (axis == "z")
                    {
                        MWBase::Environment::get().getWorld()->rotateObject(ptr,ax,ay,angle);
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
                        runtime.push(Ogre::Radian(ptr.getRefData().getPosition().rot[0]).valueDegrees());
                    }
                    else if (axis=="y")
                    {
                        runtime.push(Ogre::Radian(ptr.getRefData().getPosition().rot[1]).valueDegrees());
                    }
                    else if (axis=="z")
                    {
                        runtime.push(Ogre::Radian(ptr.getRefData().getPosition().rot[2]).valueDegrees());
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

        template<class R>
        class OpPlaceAtPc : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
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

                    ESM::Position ipos = MWBase::Environment::get().getWorld()->getPlayer().getPlayer().getRefData().getPosition();
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

                    MWWorld::CellStore* store = MWBase::Environment::get().getWorld()->getPlayer().getPlayer().getCell();                    
                    MWWorld::ManualRef ref(MWBase::Environment::get().getWorld()->getStore(),itemID);
                    ref.getPtr().getCellRef().mPos = ipos;
                    ref.getPtr().getRefData().setCount(count);
                    MWBase::Environment::get().getWorld()->safePlaceObject(ref.getPtr(),*store,ipos);
                }
        };

        template<class R>
        class OpPlaceAtMe : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr me = R()(runtime);

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

                    ESM::Position ipos = me.getRefData().getPosition();
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

                    MWWorld::CellStore* store = me.getCell();                    
                    MWWorld::ManualRef ref(MWBase::Environment::get().getWorld()->getStore(),itemID);
                    ref.getPtr().getCellRef().mPos = ipos;
                    ref.getPtr().getRefData().setCount(count);
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

                    float *objRot = ptr.getRefData().getPosition().rot;

                    if (axis == "x")
                    {
                        objRot[0]+=Ogre::Degree(rotation).valueRadians();

                        if (ptr.getRefData().getBaseNode() != 0)
                            MWBase::Environment::get().getWorld()->localRotateObject(ptr, rotation, Ogre::Vector3::UNIT_X);
                    }
                    else if (axis == "y")
                    {
                        objRot[1]+=Ogre::Degree(rotation).valueRadians();

                        if (ptr.getRefData().getBaseNode() != 0)
                        MWBase::Environment::get().getWorld()->localRotateObject(ptr, rotation, Ogre::Vector3::UNIT_Y);
                    }
                    else if (axis == "z")
                    {
                        objRot[2]+=Ogre::Degree(rotation).valueRadians();

                        if (ptr.getRefData().getBaseNode() != 0)
                        MWBase::Environment::get().getWorld()->localRotateObject(ptr, rotation, Ogre::Vector3::UNIT_Z);
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

        const int opcodeSetScale = 0x2000164;
        const int opcodeSetScaleExplicit = 0x2000165;
        const int opcodeSetAngle = 0x2000166;
        const int opcodeSetAngleExplicit = 0x2000167;
        const int opcodeGetScale = 0x2000168;
        const int opcodeGetScaleExplicit = 0x2000169;
        const int opcodeGetAngle = 0x200016a;
        const int opcodeGetAngleExplicit = 0x200016b;
        const int opcodeGetPos = 0x2000190;
        const int opcodeGetPosExplicit = 0x2000191;
        const int opcodeSetPos = 0x2000192;
        const int opcodeSetPosExplicit = 0x2000193;
        const int opcodeGetStartingPos = 0x2000194;
        const int opcodeGetStartingPosExplicit = 0x2000195;
        const int opcodePosition = 0x2000196;
        const int opcodePositionExplicit = 0x2000197;
        const int opcodePositionCell = 0x2000198;
        const int opcodePositionCellExplicit = 0x2000199;

        const int opcodePlaceItemCell = 0x200019a;
        const int opcodePlaceItem = 0x200019b;
        const int opcodePlaceAtPc = 0x200019c;  
        const int opcodePlaceAtMe = 0x200019d;
        const int opcodePlaceAtMeExplicit = 0x200019e;
        const int opcodeModScale = 0x20001e3;
        const int opcodeModScaleExplicit = 0x20001e4;
        const int opcodeRotate = 0x20001ff;
        const int opcodeRotateExplicit = 0x2000200;
        const int opcodeRotateWorld = 0x2000201;
        const int opcodeRotateWorldExplicit = 0x2000202;

        void registerExtensions (Compiler::Extensions& extensions)
        {
            extensions.registerInstruction("setscale","f",opcodeSetScale,opcodeSetScaleExplicit);
            extensions.registerFunction("getscale",'f',"",opcodeGetScale,opcodeGetScaleExplicit);
            extensions.registerInstruction("setangle","cf",opcodeSetAngle,opcodeSetAngleExplicit);
            extensions.registerFunction("getangle",'f',"c",opcodeGetAngle,opcodeGetAngleExplicit);
            extensions.registerInstruction("setpos","cf",opcodeSetPos,opcodeSetPosExplicit);
            extensions.registerFunction("getpos",'f',"c",opcodeGetPos,opcodeGetPosExplicit);
            extensions.registerFunction("getstartingpos",'f',"c",opcodeGetStartingPos,opcodeGetStartingPosExplicit);
            extensions.registerInstruction("position","ffff",opcodePosition,opcodePositionExplicit);
            extensions.registerInstruction("positioncell","ffffc",opcodePositionCell,opcodePositionCellExplicit);
            extensions.registerInstruction("placeitemcell","ccffff",opcodePlaceItemCell);
            extensions.registerInstruction("placeitem","cffff",opcodePlaceItem);
            extensions.registerInstruction("placeatpc","clfl",opcodePlaceAtPc);
            extensions.registerInstruction("placeatme","clfl",opcodePlaceAtMe,opcodePlaceAtMeExplicit);
            extensions.registerInstruction("modscale","f",opcodeModScale,opcodeModScaleExplicit);
            extensions.registerInstruction("rotate","cf",opcodeRotate,opcodeRotateExplicit);
            extensions.registerInstruction("rotateworld","cf",opcodeRotateWorld,opcodeRotateWorldExplicit);
        }

        void installOpcodes (Interpreter::Interpreter& interpreter)
        {
            interpreter.installSegment5(opcodeSetScale,new OpSetScale<ImplicitRef>);
            interpreter.installSegment5(opcodeSetScaleExplicit,new OpSetScale<ExplicitRef>);
            interpreter.installSegment5(opcodeSetAngle,new OpSetAngle<ImplicitRef>);
            interpreter.installSegment5(opcodeSetAngleExplicit,new OpSetAngle<ExplicitRef>);
            interpreter.installSegment5(opcodeGetScale,new OpGetScale<ImplicitRef>);
            interpreter.installSegment5(opcodeGetScaleExplicit,new OpGetScale<ExplicitRef>);
            interpreter.installSegment5(opcodeGetAngle,new OpGetAngle<ImplicitRef>);
            interpreter.installSegment5(opcodeGetAngleExplicit,new OpGetAngle<ExplicitRef>);
            interpreter.installSegment5(opcodeGetPos,new OpGetPos<ImplicitRef>);
            interpreter.installSegment5(opcodeGetPosExplicit,new OpGetPos<ExplicitRef>);
            interpreter.installSegment5(opcodeSetPos,new OpSetPos<ImplicitRef>);
            interpreter.installSegment5(opcodeSetPosExplicit,new OpSetPos<ExplicitRef>);
            interpreter.installSegment5(opcodeGetStartingPos,new OpGetStartingPos<ImplicitRef>);
            interpreter.installSegment5(opcodeGetStartingPosExplicit,new OpGetStartingPos<ExplicitRef>);
            interpreter.installSegment5(opcodePosition,new OpPosition<ImplicitRef>);
            interpreter.installSegment5(opcodePositionExplicit,new OpPosition<ExplicitRef>);
            interpreter.installSegment5(opcodePositionCell,new OpPositionCell<ImplicitRef>);
            interpreter.installSegment5(opcodePositionCellExplicit,new OpPositionCell<ExplicitRef>);
            interpreter.installSegment5(opcodePlaceItemCell,new OpPlaceItemCell<ImplicitRef>);            
            interpreter.installSegment5(opcodePlaceItem,new OpPlaceItem<ImplicitRef>);            
            interpreter.installSegment5(opcodePlaceAtPc,new OpPlaceAtPc<ImplicitRef>);   
            interpreter.installSegment5(opcodePlaceAtMe,new OpPlaceAtMe<ImplicitRef>);   
            interpreter.installSegment5(opcodePlaceAtMeExplicit,new OpPlaceAtMe<ExplicitRef>);
            interpreter.installSegment5(opcodeModScale,new OpModScale<ImplicitRef>);
            interpreter.installSegment5(opcodeModScaleExplicit,new OpModScale<ExplicitRef>);
            interpreter.installSegment5(opcodeRotate,new OpRotate<ImplicitRef>);
            interpreter.installSegment5(opcodeRotateExplicit,new OpRotate<ExplicitRef>);
            interpreter.installSegment5(opcodeRotateWorld,new OpRotateWorld<ImplicitRef>);
            interpreter.installSegment5(opcodeRotateWorldExplicit,new OpRotateWorld<ExplicitRef>);
        }
    }
}
