#include "generator.hpp"

#include <cassert>
#include <algorithm>
#include <iterator>
#include <stdexcept>

#include "literals.hpp"

namespace
{
    void opPushInt (Compiler::Generator::CodeContainer& code, int value)
    {
        code.push_back (Compiler::Generator::segment0 (0, value));
    }

    void opFetchIntLiteral (Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Compiler::Generator::segment5 (4));
    }

    void opFetchFloatLiteral (Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Compiler::Generator::segment5 (5));
    }

    void opIntToFloat (Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Compiler::Generator::segment5 (3));
    }

    void opFloatToInt (Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Compiler::Generator::segment5 (6));
    }

    void opStoreLocalShort (Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Compiler::Generator::segment5 (0));
    }

    void opStoreLocalLong (Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Compiler::Generator::segment5 (1));
    }

    void opStoreLocalFloat (Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Compiler::Generator::segment5 (2));
    }

    void opNegateInt (Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Compiler::Generator::segment5 (7));
    }

    void opNegateFloat (Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Compiler::Generator::segment5 (8));
    }

    void opAddInt (Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Compiler::Generator::segment5 (9));
    }

    void opAddFloat (Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Compiler::Generator::segment5 (10));
    }

    void opSubInt (Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Compiler::Generator::segment5 (11));
    }

    void opSubFloat (Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Compiler::Generator::segment5 (12));
    }

    void opMulInt (Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Compiler::Generator::segment5 (13));
    }

    void opMulFloat (Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Compiler::Generator::segment5 (14));
    }

    void opDivInt (Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Compiler::Generator::segment5 (15));
    }

    void opDivFloat (Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Compiler::Generator::segment5 (16));
    }

    void opIntToFloat1 (Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Compiler::Generator::segment5 (17));
    }

    void opSquareRoot (Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Compiler::Generator::segment5 (19));
    }

    void opReturn (Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Compiler::Generator::segment5 (20));
    }

    void opMessageBox (Compiler::Generator::CodeContainer& code, int buttons)
    {
        code.push_back (Compiler::Generator::segment3 (0, buttons));
    }

    void opReport (Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Compiler::Generator::segment5 (58));
    }

    void opFetchLocalShort (Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Compiler::Generator::segment5 (21));
    }

    void opFetchLocalLong (Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Compiler::Generator::segment5 (22));
    }

    void opFetchLocalFloat (Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Compiler::Generator::segment5 (23));
    }

    void opJumpForward (Compiler::Generator::CodeContainer& code, int offset)
    {
        code.push_back (Compiler::Generator::segment0 (1, offset));
    }

    void opJumpBackward (Compiler::Generator::CodeContainer& code, int offset)
    {
        code.push_back (Compiler::Generator::segment0 (2, offset));
    }

    /*
    Currently unused
    void opSkipOnZero (Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Compiler::Generator::segment5 (24));
    }
    */

    void opSkipOnNonZero (Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Compiler::Generator::segment5 (25));
    }

    void opEqualInt (Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Compiler::Generator::segment5 (26));
    }

    void opNonEqualInt (Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Compiler::Generator::segment5 (27));
    }

    void opLessThanInt (Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Compiler::Generator::segment5 (28));
    }

    void opLessOrEqualInt (Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Compiler::Generator::segment5 (29));
    }

    void opGreaterThanInt (Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Compiler::Generator::segment5 (30));
    }

    void opGreaterOrEqualInt (Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Compiler::Generator::segment5 (31));
    }

    void opEqualFloat (Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Compiler::Generator::segment5 (32));
    }

    void opNonEqualFloat (Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Compiler::Generator::segment5 (33));
    }

    void opLessThanFloat (Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Compiler::Generator::segment5 (34));
    }

    void opLessOrEqualFloat (Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Compiler::Generator::segment5 (35));
    }

    void opGreaterThanFloat (Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Compiler::Generator::segment5 (36));
    }

    void opGreaterOrEqualFloat (Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Compiler::Generator::segment5 (37));
    }

    void opStoreGlobalShort (Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Compiler::Generator::segment5 (39));
    }

    void opStoreGlobalLong (Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Compiler::Generator::segment5 (40));
    }

    void opStoreGlobalFloat (Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Compiler::Generator::segment5 (41));
    }

    void opFetchGlobalShort (Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Compiler::Generator::segment5 (42));
    }

    void opFetchGlobalLong (Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Compiler::Generator::segment5 (43));
    }

    void opFetchGlobalFloat (Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Compiler::Generator::segment5 (44));
    }

    void opStoreMemberShort (Compiler::Generator::CodeContainer& code, bool global)
    {
        code.push_back (Compiler::Generator::segment5 (global ? 65 : 59));
    }

    void opStoreMemberLong (Compiler::Generator::CodeContainer& code, bool global)
    {
        code.push_back (Compiler::Generator::segment5 (global ? 66 : 60));
    }

    void opStoreMemberFloat (Compiler::Generator::CodeContainer& code, bool global)
    {
        code.push_back (Compiler::Generator::segment5 (global ? 67 : 61));
    }

    void opFetchMemberShort (Compiler::Generator::CodeContainer& code, bool global)
    {
        code.push_back (Compiler::Generator::segment5 (global ? 68 : 62));
    }

    void opFetchMemberLong (Compiler::Generator::CodeContainer& code, bool global)
    {
        code.push_back (Compiler::Generator::segment5 (global ? 69 : 63));
    }

    void opFetchMemberFloat (Compiler::Generator::CodeContainer& code, bool global)
    {
        code.push_back (Compiler::Generator::segment5 (global ? 70 : 64));
    }
}

namespace Compiler::Generator
    {
        void pushInt (CodeContainer& code, Literals& literals, int value)
        {
            int index = literals.addInteger (value);
            opPushInt (code, index);
            opFetchIntLiteral (code);
        }

        void pushFloat (CodeContainer& code, Literals& literals, float value)
        {
            int index = literals.addFloat (value);
            opPushInt (code, index);
            opFetchFloatLiteral (code);
        }

        void pushString (CodeContainer& code, Literals& literals, const std::string& value)
        {
            int index = literals.addString (value);
            opPushInt (code, index);
        }

        void assignToLocal (CodeContainer& code, char localType,
            int localIndex, const CodeContainer& value, char valueType)
        {
            opPushInt (code, localIndex);

            std::copy (value.begin(), value.end(), std::back_inserter (code));

            if (localType!=valueType)
            {
                if (localType=='f' && valueType=='l')
                {
                    opIntToFloat (code);
                }
                else if ((localType=='l' || localType=='s') && valueType=='f')
                {
                    opFloatToInt (code);
                }
            }

            switch (localType)
            {
                case 'f':

                    opStoreLocalFloat (code);
                    break;

                case 's':

                    opStoreLocalShort (code);
                    break;

                case 'l':

                    opStoreLocalLong (code);
                    break;

                default:

                    assert (0);
            }
        }

        void negate (CodeContainer& code, char valueType)
        {
            switch (valueType)
            {
                case 'l':

                    opNegateInt (code);
                    break;

                case 'f':

                    opNegateFloat (code);
                    break;

                default:

                    assert (0);
            }
        }

        void add (CodeContainer& code, char valueType1, char valueType2)
        {
            if (valueType1=='l' && valueType2=='l')
            {
                opAddInt (code);
            }
            else
            {
                if (valueType1=='l')
                    opIntToFloat1 (code);

                if (valueType2=='l')
                    opIntToFloat (code);

                opAddFloat (code);
            }
        }

        void sub (CodeContainer& code, char valueType1, char valueType2)
        {
            if (valueType1=='l' && valueType2=='l')
            {
                opSubInt (code);
            }
            else
            {
                if (valueType1=='l')
                    opIntToFloat1 (code);

                if (valueType2=='l')
                    opIntToFloat (code);

                opSubFloat (code);
            }
        }

        void mul (CodeContainer& code, char valueType1, char valueType2)
        {
            if (valueType1=='l' && valueType2=='l')
            {
                opMulInt (code);
            }
            else
            {
                if (valueType1=='l')
                    opIntToFloat1 (code);

                if (valueType2=='l')
                    opIntToFloat (code);

                opMulFloat (code);
            }
        }

        void div (CodeContainer& code, char valueType1, char valueType2)
        {
            if (valueType1=='l' && valueType2=='l')
            {
                opDivInt (code);
            }
            else
            {
                if (valueType1=='l')
                    opIntToFloat1 (code);

                if (valueType2=='l')
                    opIntToFloat (code);

                opDivFloat (code);
            }
        }

        void convert (CodeContainer& code, char fromType, char toType)
        {
            if (fromType!=toType)
            {
                if (fromType=='f' && toType=='l')
                    opFloatToInt (code);
                else if (fromType=='l' && toType=='f')
                    opIntToFloat (code);
                else
                    throw std::logic_error ("illegal type conversion");
            }
        }

        void squareRoot (CodeContainer& code)
        {
            opSquareRoot (code);
        }

        void exit (CodeContainer& code)
        {
            opReturn (code);
        }

        void message (CodeContainer& code, Literals& literals, const std::string& message,
            int buttons)
        {
            assert (buttons>=0);

            if (buttons>=256)
                throw std::runtime_error ("A message box can't have more than 255 buttons");

            int index = literals.addString (message);

            opPushInt (code, index);
            opMessageBox (code, buttons);
        }

        void report (CodeContainer& code, Literals& literals, const std::string& message)
        {
            int index = literals.addString (message);

            opPushInt (code, index);
            opReport (code);
        }

        void fetchLocal (CodeContainer& code, char localType, int localIndex)
        {
            opPushInt (code, localIndex);

            switch (localType)
            {
                case 'f':

                    opFetchLocalFloat (code);
                    break;

                case 's':

                    opFetchLocalShort (code);
                    break;

                case 'l':

                    opFetchLocalLong (code);
                    break;

                default:

                    assert (0);
            }
        }

        void jump (CodeContainer& code, int offset)
        {
            if (offset>0)
                opJumpForward (code, offset);
            else if (offset<0)
                opJumpBackward (code, -offset);
            else
                throw std::logic_error ("infinite loop");
        }

        void jumpOnZero (CodeContainer& code, int offset)
        {
            opSkipOnNonZero (code);

            if (offset<0)
                --offset; // compensate for skip instruction

            jump (code, offset);
        }

        void compare (CodeContainer& code, char op, char valueType1, char valueType2)
        {
            if (valueType1=='l' && valueType2=='l')
            {
                switch (op)
                {
                    case 'e': opEqualInt (code); break;
                    case 'n': opNonEqualInt (code); break;
                    case 'l': opLessThanInt (code); break;
                    case 'L': opLessOrEqualInt (code); break;
                    case 'g': opGreaterThanInt (code); break;
                    case 'G': opGreaterOrEqualInt (code); break;

                    default:

                        assert (0);
                }
            }
            else
            {
                if (valueType1=='l')
                    opIntToFloat1 (code);

                if (valueType2=='l')
                    opIntToFloat (code);

                switch (op)
                {
                    case 'e': opEqualFloat (code); break;
                    case 'n': opNonEqualFloat (code); break;
                    case 'l': opLessThanFloat (code); break;
                    case 'L': opLessOrEqualFloat (code); break;
                    case 'g': opGreaterThanFloat (code); break;
                    case 'G': opGreaterOrEqualFloat (code); break;

                    default:

                        assert (0);
                }
            }
        }

        void assignToGlobal (CodeContainer& code, Literals& literals, char localType,
            const std::string& name, const CodeContainer& value, char valueType)
        {
            int index = literals.addString (name);

            opPushInt (code, index);

            std::copy (value.begin(), value.end(), std::back_inserter (code));

            if (localType!=valueType)
            {
                if (localType=='f' && (valueType=='l' || valueType=='s'))
                {
                    opIntToFloat (code);
                }
                else if ((localType=='l' || localType=='s') && valueType=='f')
                {
                    opFloatToInt (code);
                }
            }

            switch (localType)
            {
                case 'f':

                    opStoreGlobalFloat (code);
                    break;

                case 's':

                    opStoreGlobalShort (code);
                    break;

                case 'l':

                    opStoreGlobalLong (code);
                    break;

                default:

                    assert (0);
            }
        }

        void fetchGlobal (CodeContainer& code, Literals& literals, char localType,
            const std::string& name)
        {
            int index = literals.addString (name);

            opPushInt (code, index);

            switch (localType)
            {
                case 'f':

                    opFetchGlobalFloat (code);
                    break;

                case 's':

                    opFetchGlobalShort (code);
                    break;

                case 'l':

                    opFetchGlobalLong (code);
                    break;

                default:

                    assert (0);
            }
        }

        void assignToMember (CodeContainer& code, Literals& literals, char localType,
            const std::string& name, const std::string& id, const CodeContainer& value,
            char valueType, bool global)
        {
            int index = literals.addString (name);

            opPushInt (code, index);

            index = literals.addString (id);

            opPushInt (code, index);

            std::copy (value.begin(), value.end(), std::back_inserter (code));

            if (localType!=valueType)
            {
                if (localType=='f' && (valueType=='l' || valueType=='s'))
                {
                    opIntToFloat (code);
                }
                else if ((localType=='l' || localType=='s') && valueType=='f')
                {
                    opFloatToInt (code);
                }
            }

            switch (localType)
            {
                case 'f':

                    opStoreMemberFloat (code, global);
                    break;

                case 's':

                    opStoreMemberShort (code, global);
                    break;

                case 'l':

                    opStoreMemberLong (code, global);
                    break;

                default:

                    assert (0);
            }
        }

        void fetchMember (CodeContainer& code, Literals& literals, char localType,
            const std::string& name, const std::string& id, bool global)
        {
            int index = literals.addString (name);

            opPushInt (code, index);

            index = literals.addString (id);

            opPushInt (code, index);

            switch (localType)
            {
                case 'f':

                    opFetchMemberFloat (code, global);
                    break;

                case 's':

                    opFetchMemberShort (code, global);
                    break;

                case 'l':

                    opFetchMemberLong (code, global);
                    break;

                default:

                    assert (0);
            }
        }
    }
