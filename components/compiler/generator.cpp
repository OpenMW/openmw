
#include "generator.hpp"

#include <cassert>
#include <algorithm>
#include <iterator>
#include <stdexcept>

#include "literals.hpp"

namespace
{
    Interpreter::Type_Code segment0 (unsigned int c, unsigned int arg0)
    {
        assert (c<64);
        return (c<<24) | (arg0 & 0xffffff);
    }

    Interpreter::Type_Code segment1 (unsigned int c, unsigned int arg0, unsigned int arg1)
    {
        assert (c<64);
        return 0x40000000 | (c<<24) | ((arg0 & 0xfff)<<12) | (arg1 & 0xfff);
    }

    Interpreter::Type_Code segment2 (unsigned int c, unsigned int arg0)
    {
        assert (c<1024);
        return 0x80000000 | (c<<20) | (arg0 & 0xfffff);
    }

    Interpreter::Type_Code segment3 (unsigned int c, unsigned int arg0)
    {
        assert (c<1024);
        return 0xc0000000 | (c<<20) | (arg0 & 0xffff);    
    }

    Interpreter::Type_Code segment4 (unsigned int c, unsigned int arg0, unsigned int arg1)
    {
        assert (c<1024);
        return 0xc4000000 | (c<<16) | ((arg0 & 0xff)<<8) | (arg1 & 0xff);
    }

    Interpreter::Type_Code segment5 (unsigned int c)
    {
        assert (c<67108864);
        return 0xc8000000 | c;
    }
    
    void opPushInt (Compiler::Generator::CodeContainer& code, int value)
    {
        code.push_back (segment0 (0, value));
    }
    
    void opFetchIntLiteral (Compiler::Generator::CodeContainer& code)
    {
        code.push_back (segment5 (4));
    }
    
    void opFetchFloatLiteral (Compiler::Generator::CodeContainer& code)
    {
        code.push_back (segment5 (5));
    }
    
    void opIntToFloat (Compiler::Generator::CodeContainer& code)
    {
        code.push_back (segment5 (3));    
    }
    
    void opFloatToInt (Compiler::Generator::CodeContainer& code)
    {
        code.push_back (segment5 (6));
    }
        
    void opStoreLocalShort (Compiler::Generator::CodeContainer& code)
    {
        code.push_back (segment5 (0));    
    }
    
    void opStoreLocalLong (Compiler::Generator::CodeContainer& code)
    {
        code.push_back (segment5 (1));
    }
    
    void opStoreLocalFloat (Compiler::Generator::CodeContainer& code)
    {
        code.push_back (segment5 (2));
    }        

    void opNegateInt (Compiler::Generator::CodeContainer& code)
    {
        code.push_back (segment5 (7));
    }

    void opNegateFloat (Compiler::Generator::CodeContainer& code)
    {
        code.push_back (segment5 (8));
    }

    void opAddInt (Compiler::Generator::CodeContainer& code)
    {
        code.push_back (segment5 (9));
    }

    void opAddFloat (Compiler::Generator::CodeContainer& code)
    {
        code.push_back (segment5 (10));
    }

    void opSubInt (Compiler::Generator::CodeContainer& code)
    {
        code.push_back (segment5 (11));
    }

    void opSubFloat (Compiler::Generator::CodeContainer& code)
    {
        code.push_back (segment5 (12));
    }

    void opMulInt (Compiler::Generator::CodeContainer& code)
    {
        code.push_back (segment5 (13));
    }

    void opMulFloat (Compiler::Generator::CodeContainer& code)
    {
        code.push_back (segment5 (14));
    }

    void opDivInt (Compiler::Generator::CodeContainer& code)
    {
        code.push_back (segment5 (15));
    }

    void opDivFloat (Compiler::Generator::CodeContainer& code)
    {
        code.push_back (segment5 (16));
    }
    
    void opIntToFloat1 (Compiler::Generator::CodeContainer& code)
    {
        code.push_back (segment5 (17));    
    }
    
    void opFloatToInt1 (Compiler::Generator::CodeContainer& code)
    {
        code.push_back (segment5 (18));
    }    

    void opSquareRoot (Compiler::Generator::CodeContainer& code)
    {
        code.push_back (segment5 (19));
    }    
}

namespace Compiler
{
    namespace Generator
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
    }
}

