
#include "installopcodes.hpp"

#include "interpreter.hpp"
#include "genericopcodes.hpp"
#include "localopcodes.hpp"

namespace Interpreter
{
    void installOpcodes (Interpreter& interpreter)
    {
        // generic
        interpreter.installSegment0 (0, new OpPushInt);
        interpreter.installSegment5 (3, new OpIntToFloat);
        interpreter.installSegment5 (6, new OpFloatToInt);
        interpreter.installSegment5 (7, new OpNegateInt);
        interpreter.installSegment5 (8, new OpNegateFloat);
            
        // local variables
        interpreter.installSegment5 (0, new OpStoreLocalShort);
        interpreter.installSegment5 (1, new OpStoreLocalLong);
        interpreter.installSegment5 (2, new OpStoreLocalFloat);        
        interpreter.installSegment5 (4, new OpFetchIntLiteral);            
        interpreter.installSegment5 (5, new OpFetchFloatLiteral);  
    }
}

