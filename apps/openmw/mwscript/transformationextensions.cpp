#include <boost/algorithm/string.hpp>

#include <components/esm_store/store.hpp>

#include <components/compiler/extensions.hpp>

#include <components/interpreter/interpreter.hpp>
#include <components/interpreter/runtime.hpp>
#include <components/interpreter/opcodes.hpp>

#include "../mwbase/environment.hpp"

#include "../mwworld/class.hpp"

#include "interpretercontext.hpp"
#include "ref.hpp"
#include "OgreSceneNode.h"

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
                    runtime.push(ptr.getCellRef().scale);
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

                    if(axis == "X")
                    {
                        MWBase::Environment::get().getWorld()->rotateObject(ptr,angle,ay,az);
                    }
                    if(axis == "Y")
                    {
                        MWBase::Environment::get().getWorld()->rotateObject(ptr,ax,angle,az);
                    }
                    if(axis == "Z")
                    {
                        MWBase::Environment::get().getWorld()->rotateObject(ptr,ax,ay,angle);
                    }
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

                    if(axis == "X")
                    {
                        runtime.push(Ogre::Radian(ptr.getRefData().getPosition().rot[0]).valueDegrees());
                    }
                    if(axis == "Y")
                    {
                        runtime.push(Ogre::Radian(ptr.getRefData().getPosition().rot[1]).valueDegrees());
                    }
                    if(axis == "Z")
                    {
                        runtime.push(Ogre::Radian(ptr.getRefData().getPosition().rot[2]).valueDegrees());
                    }
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
                    
                    if(axis == "X")
                    {
                        runtime.push(Ogre::Radian(ptr.getCellRef().pos.rot[0]).valueDegrees());
                    }
                    if(axis == "Y")
                    {
                        runtime.push(Ogre::Radian(ptr.getCellRef().pos.rot[1]).valueDegrees());
                    }
                    if(axis == "Z")
                    {
                        runtime.push(Ogre::Radian(ptr.getCellRef().pos.rot[2]).valueDegrees());
                    }
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
        const int opcodeGetStartingAngle = 0x200016c;
        const int opcodeGetStartingAngleExplicit = 0x200016d;

        void registerExtensions (Compiler::Extensions& extensions)
        {
            extensions.registerInstruction("setscale","f",opcodeSetScale,opcodeSetScaleExplicit);
            extensions.registerFunction("getscale",'f',"",opcodeGetScale,opcodeGetScaleExplicit);
            extensions.registerInstruction("setangle","Sf",opcodeSetAngle,opcodeSetAngleExplicit);
            extensions.registerFunction("getangle",'f',"S",opcodeGetAngle,opcodeGetAngleExplicit);
            extensions.registerFunction("getstartingangle",'f',"S",opcodeGetStartingAngle,opcodeGetStartingAngleExplicit);
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
            interpreter.installSegment5(opcodeGetStartingAngle,new OpGetStartingAngle<ImplicitRef>);
            interpreter.installSegment5(opcodeGetStartingAngleExplicit,new OpGetStartingAngle<ExplicitRef>);
        }
    }
}
