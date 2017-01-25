set(JNI_ANDROID
    vmstorage.cpp
    androidcontrolshider.cpp
    android_commandLine.cpp
    android_main.c
)
source_group(android FILES ${JNI_ANDROID})

add_library(jni_android
        STATIC
        ${JNI_ANDROID}
)
set_target_properties(jni_android PROPERTIES LINKER_LANGUAGE CXX)
