# Try to find TinyXML library

# Once done this will define
#  TinyXML_FOUND         - System has the all required components.
#  TinyXML_INCLUDE_DIRS  - Include directory necessary for using the required components headers.
#  TinyXML_LIBRARIES     - Link these to use TinyXML.
#

include(LibFindMacros)

libfind_pkg_detect(TinyXML tinyxml
    FIND_PATH tinyxml.h
    FIND_LIBRARY tinyxml
)
libfind_version_n_header(TinyXML
    NAMES tinyxml.h
    CONSTANTS TIXML_MAJOR_VERSION TIXML_MINOR_VERSION TIXML_PATCH_VERSION
)
libfind_process(TinyXML)
