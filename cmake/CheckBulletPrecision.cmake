set(TMP_ROOT ${CMAKE_BINARY_DIR}/try-compile)
file(MAKE_DIRECTORY ${TMP_ROOT})

file(WRITE ${TMP_ROOT}/checkbullet.cpp
"
#include <BulletCollision/CollisionShapes/btSphereShape.h>
int main(int argc, char** argv)
{
    btSphereShape shape(1.0);
    btScalar mass(1.0);
    btVector3 inertia;
    shape.calculateLocalInertia(mass, inertia);
    return 0;
}
")

file(WRITE ${TMP_ROOT}/CMakeLists.txt
"
cmake_minimum_required(VERSION 3.1.0)
project(checkbullet)
add_executable(checkbullet checkbullet.cpp)
find_package(Bullet REQUIRED COMPONENTS BulletCollision LinearMath)
target_compile_definitions(checkbullet PUBLIC BT_USE_DOUBLE_PRECISION)
include_directories(\$\{BULLET_INCLUDE_DIRS\})
include(${CMAKE_SOURCE_DIR}/cmake/OSIdentity.cmake)
if (UBUNTU_FOUND OR DEBIAN_FOUND)
    target_link_libraries(checkbullet BulletCollision-float64 LinearMath-float64)
else()
    target_link_libraries(checkbullet \$\{BULLET_LIBRARIES\})
endif()
")

if (DEFINED BULLET_ROOT)
    set(ROOT "-DBULLET_ROOT=${BULLET_ROOT}")
endif()

message(STATUS "Checking if Bullet uses double precision")

try_compile(RESULT_VAR
    ${CMAKE_BINARY_DIR}/temp
    ${TMP_ROOT}
    checkbullet
    CMAKE_FLAGS  "${ROOT}"
    )
set(HAS_DOUBLE_PRECISION_BULLET ${RESULT_VAR})
