# math(EXPR "...") can't parse hex until 3.13
cmake_minimum_required(VERSION 3.13)

if (NOT DEFINED INPUT_FILE)
    message(STATUS "Usage: cmake -DINPUT_FILE=\"infile.ext\" -DOUTPUT_FILE=\"out.txt\" -P base64.cmake")
    message(FATAL_ERROR "INPUT_FILE not specified")
endif()

if (NOT DEFINED OUTPUT_FILE)
    message(STATUS "Usage: cmake -DINPUT_FILE=\"infile.ext\" -DOUTPUT_FILE=\"out.txt\" -P base64.cmake")
    message(FATAL_ERROR "OUTPUT_FILE not specified")
endif()

if (NOT EXISTS ${INPUT_FILE})
    message(FATAL_ERROR "INPUT_FILE ${INPUT_FILE} does not exist")
endif()

set(lut "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/")

file(READ "${INPUT_FILE}" hexContent HEX)

set(base64Content "")
while(TRUE)
    string(LENGTH "${hexContent}" tailLength)
    if (tailLength LESS 1)
        break()
    endif()

    string(SUBSTRING "${hexContent}" 0 6 head)
    # base64 works on three-byte chunks. Pad.
    string(LENGTH "${head}" headLength)
    if (headLength LESS 6)
        set(hexContent "")
        math(EXPR padSize "6 - ${headLength}")
        set(pad "")
        foreach(i RANGE 1 ${padSize})
            string(APPEND pad "0")
        endforeach()
        string(APPEND head "${pad}")
    else()
        string(SUBSTRING "${hexContent}" 6 -1 hexContent)
        set(padSize 0)   
    endif()

    # get six-bit slices
    math(EXPR first "0x${head} >> 18")
    math(EXPR second "(0x${head} & 0x3F000) >> 12")
    math(EXPR third "(0x${head} & 0xFC0) >> 6")
    math(EXPR fourth "0x${head} & 0x3F")

    # first two characters are always needed to represent the first byte
    string(SUBSTRING "${lut}" ${first} 1 char)
    string(APPEND base64Content "${char}")
    string(SUBSTRING "${lut}" ${second} 1 char)
    string(APPEND base64Content "${char}")

    # if there's no second byte, pad with =
    if (NOT padSize EQUAL 4)
        string(SUBSTRING "${lut}" ${third} 1 char)
        string(APPEND base64Content "${char}")
    else()
        string(APPEND base64Content "=")
    endif()

    # if there's no third byte, pad with =
    if (padSize EQUAL 0)
        string(SUBSTRING "${lut}" ${fourth} 1 char)
        string(APPEND base64Content "${char}")
    else()
        string(APPEND base64Content "=")
    endif()
endwhile()

file(WRITE "${OUTPUT_FILE}" "${base64Content}")