set(TMP_ROOT ${CMAKE_BINARY_DIR}/try-compile)
file(MAKE_DIRECTORY ${TMP_ROOT})

file(WRITE ${TMP_ROOT}/checkmultiview.cpp
"
#include <osg/Camera>
int main(void)
{
    (void)osg::Camera::FACE_CONTROLLED_BY_MULTIVIEW_SHADER;
    return 0;
}
")

message(STATUS "Checking if OSG supports multiview")

try_compile(RESULT_VAR
    ${TMP_ROOT}/temp
    ${TMP_ROOT}/checkmultiview.cpp
    CMAKE_FLAGS  "-DINCLUDE_DIRECTORIES=${OPENSCENEGRAPH_INCLUDE_DIRS}"
    )
set(HAVE_MULTIVIEW ${RESULT_VAR})
if(HAVE_MULTIVIEW)
    message(STATUS "Osg supports multiview")
else(HAVE_MULTIVIEW)
    message(NOTICE "Osg does not support multiview, disabling use of GL_OVR_multiview")
endif(HAVE_MULTIVIEW)
