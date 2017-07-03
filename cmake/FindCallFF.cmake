FIND_PATH(CallFF_INCLUDES call.hpp
        ENV CPATH
        /usr/include
        /usr/local/include
        /opt/local/include
        $ENV{CallFF_ROOT}/include
        )

FIND_LIBRARY(CallFF_LIBRARY NAMES callff
        PATHS
        ENV LD_LIBRARY_PATH
        ENV LIBRARY_PATH
        /usr/lib64
        /usr/lib
        /usr/local/lib64
        /usr/local/lib
        /opt/local/lib
        $ENV{CallFF_ROOT}/lib
        )

if(CallFF_LIBRARY)
    set(CallFF_LIBRARIES "${CallFF_LIBRARY}" CACHE STRING "CallFF Libraries")
endif()

if(CallFF_INCLUDES AND CallFF_LIBRARY)
    set(CallFF_FOUND TRUE)
endif(CallFF_INCLUDES AND CallFF_LIBRARY)
