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

# First, try BulletConfig-float64.cmake which comes with Debian derivatives.
find_package(Bullet CONFIGS BulletConfig-float64.cmake QUIET COMPONENTS BulletCollision LinearMath)
if (BULLET_FOUND)
    # Fix the relative include:
    set(BULLET_INCLUDE_DIRS \"\$\{BULLET_ROOT_DIR\}/\$\{BULLET_INCLUDE_DIRS\}\")
else()
    find_package(Bullet REQUIRED COMPONENTS BulletCollision LinearMath)
endif()
string(REGEX MATCHALL \"((optimized|debug);)?[^;]*(BulletCollision|LinearMath)[^;]*\" BULLET_LIBRARIES \"$\{BULLET_LIBRARIES\}\")

add_executable(checkbullet checkbullet.cpp)
target_compile_definitions(checkbullet PUBLIC BT_USE_DOUBLE_PRECISION)
target_include_directories(checkbullet PUBLIC \$\{BULLET_INCLUDE_DIRS\})
target_link_libraries(checkbullet \$\{BULLET_LIBRARIES\})
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
