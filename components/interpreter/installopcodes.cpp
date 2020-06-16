#include "installopcodes.hpp"

#include <functional>

#include "interpreter.hpp"
#include "genericopcodes.hpp"
#include "localopcodes.hpp"
#include "mathopcodes.hpp"
#include "controlopcodes.hpp"
#include "miscopcodes.hpp"

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
        interpreter.installSegment5 (17, new OpIntToFloat1);
        interpreter.installSegment5 (18, new OpFloatToInt1);

        // local variables, global variables & literals
        interpreter.installSegment5 (0, new OpStoreLocalShort);
        interpreter.installSegment5 (1, new OpStoreLocalLong);
        interpreter.installSegment5 (2, new OpStoreLocalFloat);
        interpreter.installSegment5 (4, new OpFetchIntLiteral);
        interpreter.installSegment5 (5, new OpFetchFloatLiteral);
        interpreter.installSegment5 (21, new OpFetchLocalShort);
        interpreter.installSegment5 (22, new OpFetchLocalLong);
        interpreter.installSegment5 (23, new OpFetchLocalFloat);
        interpreter.installSegment5 (39, new OpStoreGlobalShort);
        interpreter.installSegment5 (40, new OpStoreGlobalLong);
        interpreter.installSegment5 (41, new OpStoreGlobalFloat);
        interpreter.installSegment5 (42, new OpFetchGlobalShort);
        interpreter.installSegment5 (43, new OpFetchGlobalLong);
        interpreter.installSegment5 (44, new OpFetchGlobalFloat);
        interpreter.installSegment5 (59, new OpStoreMemberShort (false));
        interpreter.installSegment5 (60, new OpStoreMemberLong (false));
        interpreter.installSegment5 (61, new OpStoreMemberFloat (false));
        interpreter.installSegment5 (62, new OpFetchMemberShort (false));
        interpreter.installSegment5 (63, new OpFetchMemberLong (false));
        interpreter.installSegment5 (64, new OpFetchMemberFloat (false));
        interpreter.installSegment5 (65, new OpStoreMemberShort (true));
        interpreter.installSegment5 (66, new OpStoreMemberLong (true));
        interpreter.installSegment5 (67, new OpStoreMemberFloat (true));
        interpreter.installSegment5 (68, new OpFetchMemberShort (true));
        interpreter.installSegment5 (69, new OpFetchMemberLong (true));
        interpreter.installSegment5 (70, new OpFetchMemberFloat (true));

        // math
        interpreter.installSegment5 (9, new OpAddInt<Type_Integer>);
        interpreter.installSegment5 (10, new OpAddInt<Type_Float>);
        interpreter.installSegment5 (11, new OpSubInt<Type_Integer>);
        interpreter.installSegment5 (12, new OpSubInt<Type_Float>);
        interpreter.installSegment5 (13, new OpMulInt<Type_Integer>);
        interpreter.installSegment5 (14, new OpMulInt<Type_Float>);
        interpreter.installSegment5 (15, new OpDivInt<Type_Integer>);
        interpreter.installSegment5 (16, new OpDivInt<Type_Float>);
        interpreter.installSegment5 (19, new OpSquareRoot);
        interpreter.installSegment5 (26,
            new OpCompare<Type_Integer, std::equal_to<Type_Integer> >);
        interpreter.installSegment5 (27,
            new OpCompare<Type_Integer, std::not_equal_to<Type_Integer> >);
        interpreter.installSegment5 (28,
            new OpCompare<Type_Integer, std::less<Type_Integer> >);
        interpreter.installSegment5 (29,
            new OpCompare<Type_Integer, std::less_equal<Type_Integer> >);
        interpreter.installSegment5 (30,
            new OpCompare<Type_Integer, std::greater<Type_Integer> >);
        interpreter.installSegment5 (31,
            new OpCompare<Type_Integer, std::greater_equal<Type_Integer> >);

        interpreter.installSegment5 (32,
            new OpCompare<Type_Float, std::equal_to<Type_Float> >);
        interpreter.installSegment5 (33,
            new OpCompare<Type_Float, std::not_equal_to<Type_Float> >);
        interpreter.installSegment5 (34,
            new OpCompare<Type_Float, std::less<Type_Float> >);
        interpreter.installSegment5 (35,
            new OpCompare<Type_Float, std::less_equal<Type_Float> >);
        interpreter.installSegment5 (36,
            new OpCompare<Type_Float, std::greater<Type_Float> >);
        interpreter.installSegment5 (37,
            new OpCompare<Type_Float, std::greater_equal<Type_Float> >);

        // control structures
        interpreter.installSegment5 (20, new OpReturn);
        interpreter.installSegment5 (24, new OpSkipZero);
        interpreter.installSegment5 (25, new OpSkipNonZero);
        interpreter.installSegment0 (1, new OpJumpForward);
        interpreter.installSegment0 (2, new OpJumpBackward);

        // misc
        interpreter.installSegment3 (0, new OpMessageBox);
        interpreter.installSegment5 (58, new OpReport);
    }
}
