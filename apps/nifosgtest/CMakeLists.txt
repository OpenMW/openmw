set (FILES
    test.cpp
)

add_executable (test ${FILES})

target_link_libraries (test ${OPENSCENEGRAPH_LIBRARIES} "components")
