# There's still some brokenness here.  OSG will load:
#   .../osgPlugins-3.5.1/libOpenMW.so
#   .../osgPlugins-3.5.1/osgdb_serializers_openmw.so
#
# OSG always loads both libraries.  It will look in many more places
# for each if these two if they are not found, so for performance
# reasons, we should supply both.  Presumably, the former is for the
# relevant code, while the latter is just the serializers.  Also,
# CMake has helpfully prefixed "lib" to the latter, which is not
# really desired.

# There was a very strange problem in components/serializers where
# CMake decided to just not link nifosg/nifloader.cpp.o no matter what
# I did.  Essentially the same syntax here works fine, so it's
# probably something about CMake that I don't understand or is broken.

# There was no link error because the class in question wasn't
# referenced until the dynamic library open pulled in the global
# constructors for the serializers, which had one of two failure
# modes: with libOpenMW.so, there were dl.so errors at program exit
# (probably due to double deconstruction?)  and without libOpenMW.so
# there were missing symbols at dlopen() in the serializer.  This
# configuration works.  There may be other working configurations as
# well.

set(serializer_sources
    alphacontroller
    billboardcallback
    compositestatesetupdater
    computeboundingboxcallback
    controller
    controllerfunction
    controllersource
    emitter
    flipcontroller
    frameswitch
    frametimesource
    geommorphcontroller
    gravityaffector
    growfadeaffector
    inverseworldmatrix
    keyframecontroller
    keyframeholder
    materialcolorcontroller
    nifcontrollerfunction
    nodeuserdata
    particlecoloraffector
    particleshooter
    particlesystem
    particlesystemcontroller
    planarcollider
    riggeometry
    skeleton
    statesetupdater
    staticboundingboxcallback
    textkeymapholder
    updatemorphgeometry
    updaterigbounds
    updateriggeometry
    uvcontroller
    viscontroller
    )

add_library(osgdb_serializers_openmw SHARED
    ${serializer_sources}
    )

add_library(OpenMW SHARED
    ../components/nifosg/nifloader
    ../components/nifosg/particle
    ../components/nifosg/controller
    ../components/nifosg/userdata
    ../components/sceneutil/statesetupdater
    ../components/sceneutil/skeleton
    ../components/sceneutil/riggeometry
    ../components/sceneutil/controller
    )

set_target_properties(osgdb_serializers_openmw PROPERTIES PREFIX "")
target_link_libraries(osgdb_serializers_openmw OpenMW)

target_link_libraries(OpenMW
    ${OSG_LIBRARIES}
    ${OSGPARTICLE_LIBRARIES}
    )

install(TARGETS osgdb_serializers_openmw
    # Should be set from OSG somehow
    LIBRARY DESTINATION ${OSG_PLUGIN_LIB_SEARCH_PATH}
    RUNTIME DESTINATION bin
    )

install(TARGETS OpenMW
    # Should be set from OSG somehow
    LIBRARY DESTINATION ${OSG_PLUGIN_LIB_SEARCH_PATH}
    RUNTIME DESTINATION bin
    )

# Make the variable accessible for other subdirectories
set(serializer_sources ${serializer_sources} PARENT_SCOPE)
