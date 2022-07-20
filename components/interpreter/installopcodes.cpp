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
    void installOpcodes(Interpreter& interpreter)
    {
        // generic
        interpreter.installSegment0<OpPushInt>(0);
        interpreter.installSegment5<OpIntToFloat>(3);
        interpreter.installSegment5<OpFloatToInt>(6);
        interpreter.installSegment5<OpNegateInt>(7);
        interpreter.installSegment5<OpNegateFloat>(8);
        interpreter.installSegment5<OpIntToFloat1>(17);
        interpreter.installSegment5<OpFloatToInt1>(18);

        // local variables, global variables & literals
        interpreter.installSegment5<OpStoreLocalShort>(0);
        interpreter.installSegment5<OpStoreLocalLong>(1);
        interpreter.installSegment5<OpStoreLocalFloat>(2);
        interpreter.installSegment5<OpFetchIntLiteral>(4);
        interpreter.installSegment5<OpFetchFloatLiteral>(5);
        interpreter.installSegment5<OpFetchLocalShort>(21);
        interpreter.installSegment5<OpFetchLocalLong>(22);
        interpreter.installSegment5<OpFetchLocalFloat>(23);
        interpreter.installSegment5<OpStoreGlobalShort>(39);
        interpreter.installSegment5<OpStoreGlobalLong>(40);
        interpreter.installSegment5<OpStoreGlobalFloat>(41);
        interpreter.installSegment5<OpFetchGlobalShort>(42);
        interpreter.installSegment5<OpFetchGlobalLong>(43);
        interpreter.installSegment5<OpFetchGlobalFloat>(44);
        interpreter.installSegment5<OpStoreMemberShort<false>>(59);
        interpreter.installSegment5<OpStoreMemberLong<false>>(60);
        interpreter.installSegment5<OpStoreMemberFloat<false>>(61);
        interpreter.installSegment5<OpFetchMemberShort<false>>(62);
        interpreter.installSegment5<OpFetchMemberLong<false>>(63);
        interpreter.installSegment5<OpFetchMemberFloat<false>>(64);
        interpreter.installSegment5<OpStoreMemberShort<true>>(65);
        interpreter.installSegment5<OpStoreMemberLong<true>>(66);
        interpreter.installSegment5<OpStoreMemberFloat<true>>(67);
        interpreter.installSegment5<OpFetchMemberShort<true>>(68);
        interpreter.installSegment5<OpFetchMemberLong<true>>(69);
        interpreter.installSegment5<OpFetchMemberFloat<true>>(70);

        // math
        interpreter.installSegment5<OpAddInt<Type_Integer>>(9);
        interpreter.installSegment5<OpAddInt<Type_Float>>(10);
        interpreter.installSegment5<OpSubInt<Type_Integer>>(11);
        interpreter.installSegment5<OpSubInt<Type_Float>>(12);
        interpreter.installSegment5<OpMulInt<Type_Integer>>(13);
        interpreter.installSegment5<OpMulInt<Type_Float>>(14);
        interpreter.installSegment5<OpDivInt<Type_Integer>>(15);
        interpreter.installSegment5<OpDivInt<Type_Float>>(16);
        interpreter.installSegment5<OpCompare<Type_Integer, std::equal_to<Type_Integer> >>(26);
        interpreter.installSegment5<OpCompare<Type_Integer, std::not_equal_to<Type_Integer> >>(27);
        interpreter.installSegment5<OpCompare<Type_Integer, std::less<Type_Integer> >>(28);
        interpreter.installSegment5<OpCompare<Type_Integer, std::less_equal<Type_Integer> >>(29);
        interpreter.installSegment5<OpCompare<Type_Integer, std::greater<Type_Integer> >>(30);
        interpreter.installSegment5<OpCompare<Type_Integer, std::greater_equal<Type_Integer> >>(31);

        interpreter.installSegment5<OpCompare<Type_Float, std::equal_to<Type_Float> >>(32);
        interpreter.installSegment5<OpCompare<Type_Float, std::not_equal_to<Type_Float> >>(33);
        interpreter.installSegment5<OpCompare<Type_Float, std::less<Type_Float> >>(34);
        interpreter.installSegment5<OpCompare<Type_Float, std::less_equal<Type_Float> >>(35);
        interpreter.installSegment5<OpCompare<Type_Float, std::greater<Type_Float> >>(36);
        interpreter.installSegment5<OpCompare<Type_Float, std::greater_equal<Type_Float> >>(37);

        // control structures
        interpreter.installSegment5<OpReturn>(20);
        interpreter.installSegment5<OpSkipZero>(24);
        interpreter.installSegment5<OpSkipNonZero>(25);
        interpreter.installSegment0<OpJumpForward>(1);
        interpreter.installSegment0<OpJumpBackward>(2);

        // misc
        interpreter.installSegment3<OpMessageBox>(0);
        interpreter.installSegment5<OpReport>(58);
    }
}
