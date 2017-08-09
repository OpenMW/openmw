#ifndef CSM_WORLD_DEFAULTGMSTS_H
#define CSM_WORLD_DEFAULTGMSTS_H

#include <cstddef>

namespace CSMWorld {
    namespace DefaultGmsts {
        
        const size_t FloatCount = 258;
        const size_t IntCount = 89;
        const size_t StringCount = 1174;
        
        const size_t OptionalFloatCount = 42;
        const size_t OptionalIntCount = 4;
        const size_t OptionalStringCount = 26;
        
        extern const char* Floats[];
        extern const char * Ints[];
        extern const char * Strings[];
        
        extern const char * OptionalFloats[];
        extern const char * OptionalInts[];
        extern const char * OptionalStrings[];

        extern const float FloatsDefaultValues[];
        extern const int IntsDefaultValues[];

        extern const float FloatLimits[];
        extern const int IntLimits[];

    }
}

#endif
