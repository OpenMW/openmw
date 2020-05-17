#ifndef COMPILER_GENERATOR_H_INCLUDED
#define COMPILER_GENERATOR_H_INCLUDED

#include <vector>
#include <string>
#include <cassert>

#include <components/interpreter/types.hpp>

namespace Compiler
{
    class Literals;

    namespace Generator
    {
        typedef std::vector<Interpreter::Type_Code> CodeContainer;

        inline Interpreter::Type_Code segment0 (unsigned int c, unsigned int arg0)
        {
            assert (c<64);
            return (c<<24) | (arg0 & 0xffffff);
        }

        inline Interpreter::Type_Code segment1 (unsigned int c, unsigned int arg0,
            unsigned int arg1)
        {
            assert (c<64);
            return 0x40000000 | (c<<24) | ((arg0 & 0xfff)<<12) | (arg1 & 0xfff);
        }

        inline Interpreter::Type_Code segment2 (unsigned int c, unsigned int arg0)
        {
            assert (c<1024);
            return 0x80000000 | (c<<20) | (arg0 & 0xfffff);
        }

        inline Interpreter::Type_Code segment3 (unsigned int c, unsigned int arg0)
        {
            assert (c<262144);
            return 0xc0000000 | (c<<8) | (arg0 & 0xff);
        }

        inline Interpreter::Type_Code segment4 (unsigned int c, unsigned int arg0,
            unsigned int arg1)
        {
            assert (c<1024);
            return 0xc4000000 | (c<<16) | ((arg0 & 0xff)<<8) | (arg1 & 0xff);
        }

        inline Interpreter::Type_Code segment5 (unsigned int c)
        {
            assert (c<67108864);
            return 0xc8000000 | c;
        }

        void pushInt (CodeContainer& code, Literals& literals, int value);

        void pushFloat (CodeContainer& code, Literals& literals, float value);

        void pushString (CodeContainer& code, Literals& literals, const std::string& value);

        void assignToLocal (CodeContainer& code, char localType,
            int localIndex, const CodeContainer& value, char valueType);

        void negate (CodeContainer& code, char valueType);

        void add (CodeContainer& code, char valueType1, char valueType2);

        void sub (CodeContainer& code, char valueType1, char valueType2);

        void mul (CodeContainer& code, char valueType1, char valueType2);

        void div (CodeContainer& code, char valueType1, char valueType2);

        void convert (CodeContainer& code, char fromType, char toType);

        void squareRoot (CodeContainer& code);

        void exit (CodeContainer& code);

        void message (CodeContainer& code, Literals& literals, const std::string& message,
            int buttons);

        void report (CodeContainer& code, Literals& literals, const std::string& message);

        void fetchLocal (CodeContainer& code, char localType, int localIndex);

        void jump (CodeContainer& code, int offset);

        void jumpOnZero (CodeContainer& code, int offset);

        void compare (CodeContainer& code, char op, char valueType1, char valueType2);

        void assignToGlobal (CodeContainer& code, Literals& literals, char localType,
            const std::string& name, const CodeContainer& value, char valueType);

        void fetchGlobal (CodeContainer& code, Literals& literals, char localType,
            const std::string& name);

        void assignToMember (CodeContainer& code, Literals& literals, char memberType,
            const std::string& name, const std::string& id, const CodeContainer& value, char valueType, bool global);
        ///< \param global Member of a global script instead of a script of a reference.

        void fetchMember (CodeContainer& code, Literals& literals, char memberType,
            const std::string& name, const std::string& id, bool global);
        ///< \param global Member of a global script instead of a script of a reference.
    }
}

#endif
