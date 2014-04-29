#include "mwseextensions.hpp"

#include <cmath>

#include <components/interpreter/interpreter.hpp>
#include <components/interpreter/runtime.hpp>
#include <components/interpreter/opcodes.hpp>
#include <components/compiler/extensions.hpp>
#include <components/compiler/opcodes.hpp>

#include <iostream>


namespace MWScript {
    namespace ScriptExtender {
        namespace Math {
            class OpArcCos : public Interpreter::Opcode0
            {
                public:
                    virtual void execute (Interpreter::Runtime& runtime)
                    {
                        Interpreter::Type_Float angle = std::acos(runtime[0].mFloat);

                        runtime.pop();

                        runtime.push(angle);
                    }
            };
            class OpArcSin : public Interpreter::Opcode0
            {
                public:
                    virtual void execute (Interpreter::Runtime& runtime)
                    {
                        Interpreter::Type_Float angle = std::asin(runtime[0].mFloat);

                        runtime.pop();

                        runtime.push(angle);
                    }
            };
            class OpArcTan : public Interpreter::Opcode0
            {
                public:
                    virtual void execute (Interpreter::Runtime& runtime)
                    {
                        Interpreter::Type_Float angle = std::atan(runtime[0].mFloat);

                        runtime.pop();

                        runtime.push(angle);
                    }
            };
            class OpCos : public Interpreter::Opcode0
            {
                public:
                    virtual void execute (Interpreter::Runtime& runtime)
                    {
                        Interpreter::Type_Float angle = std::cos(runtime[0].mFloat);
                        runtime.pop();

                        runtime.push(angle);
                    }
            };
            class OpSin : public Interpreter::Opcode0
            {
                public:
                    virtual void execute (Interpreter::Runtime& runtime)
                    {
                        Interpreter::Type_Float angle = std::sin(runtime[0].mFloat);

                        runtime.pop();

                        runtime.push(angle);
                    }
            };
            class OpTan : public Interpreter::Opcode0
            {
                public:
                    virtual void execute (Interpreter::Runtime& runtime)
                    {
                        Interpreter::Type_Float angle = std::atan(runtime[0].mFloat);

                        runtime.pop();

                        runtime.push(angle);
                    }
            };
            class OpHypot : public Interpreter::Opcode0
            {
                public:
                    virtual void execute (Interpreter::Runtime& runtime)
                    {
                        Interpreter::Type_Float a = runtime[0].mFloat;
                        Interpreter::Type_Float b = runtime[1].mFloat;

                        runtime.pop();

                        runtime.push(std::sqrt(a*a + b*b));
                    }
            };
            class OpDegRad : public Interpreter::Opcode0
            {
                public:
                    virtual void execute (Interpreter::Runtime& runtime)
                    {
                        Interpreter::Type_Float result = runtime[0].mFloat * 0.0174532925;

                        runtime.pop();

                        runtime.push(result);
                    }
            };
            class OpRadDeg : public Interpreter::Opcode0
            {
                public:
                    virtual void execute (Interpreter::Runtime& runtime)
                    {
                        Interpreter::Type_Float result = runtime[0].mFloat * 57.2957795;

                        runtime.pop();

                        runtime.push(result);
                    }
            };
            void installOpcodes (Interpreter::Interpreter& interpreter)
            {
                interpreter.installSegment5 (Compiler::ScriptExtender::Math::opcodeArcCos, new OpArcCos);
                interpreter.installSegment5 (Compiler::ScriptExtender::Math::opcodeArcSin, new OpArcSin);
                interpreter.installSegment5 (Compiler::ScriptExtender::Math::opcodeArcTan, new OpArcTan);
                interpreter.installSegment5 (Compiler::ScriptExtender::Math::opcodeCos, new OpCos);
                interpreter.installSegment5 (Compiler::ScriptExtender::Math::opcodeSin, new OpSin);
                interpreter.installSegment5 (Compiler::ScriptExtender::Math::opcodeTan, new OpTan);
                interpreter.installSegment5 (Compiler::ScriptExtender::Math::opcodeHypot, new OpHypot);
                interpreter.installSegment5 (Compiler::ScriptExtender::Math::opcodeDegRad, new OpDegRad);
                interpreter.installSegment5 (Compiler::ScriptExtender::Math::opcodeRadDeg, new OpRadDeg);
            }
        }
    }
}
