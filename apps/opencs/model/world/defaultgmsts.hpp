#ifndef CSM_WORLD_DEFAULTGMSTS_H
#define CSM_WORLD_DEFAULTGMSTS_H

#include <cstddef>

namespace CSMWorld {
    class DefaultGMSTs {
    public:
        
        static size_t getFloatCount();
        static size_t getIntCount();
        static size_t getStringCount();
        
        static size_t getOptFloatCount();
        static size_t getOptIntCount();
        static size_t getOptStringCount();
        
        static const char * getFloatName(size_t index);
        static const char * getIntName(size_t index);
        static const char * getStringName(size_t index);
        
        // Optional GMSTs appear to be in the large list as well
        static const char * getOptFloatName(size_t index);
        static const char * getOptIntName(size_t index);
        static const char * getOptStringName(size_t index);
        
        static float getFloatDefaultValue(size_t index);
        static int getIntDefaultValue(size_t index);
        
        static float getFloatLowerLimit(size_t index);
        static float getFloatUpperLimit(size_t index);
        
        static int getIntLowerLimit(size_t index);
        static int getIntUpperLimit(size_t index);
        
    private:
        
        static const char * mGMSTFloats[];
        static const char * mGMSTInts[];
        static const char * mGMSTStrings[];

        static const char * mGMSTOptionalFloats[];
        static const char * mGMSTOptionalInts[];
        static const char * mGMSTOptionalStrings[];
        
        static const float mGMSTFloatsDefaultValues[];
        static const int mGMSTIntsDefaultValues[];
        
        static const float mGMSTFloatLimits[];
        static const int mGMSTIntLimits[];
    };
}

#endif
