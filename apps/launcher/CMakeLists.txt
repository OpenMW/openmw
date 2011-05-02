set(LAUNCHER
  datafilespage.cpp
  lineedit.cpp
  main.cpp
  maindialog.cpp
  playpage.cpp

  datafilespage.hpp
  lineedit.hpp
  maindialog.hpp
  playpage.hpp
  combobox.hpp
)

set(MOC_HDRS
  datafilespage.hpp
  lineedit.hpp
  maindialog.hpp
  playpage.hpp
  combobox.hpp
)

find_package(Qt4 REQUIRED)
set(QT_USE_QTGUI 1)

QT4_ADD_RESOURCES(RCC_SRCS resources.qrc)
QT4_WRAP_CPP(MOC_SRCS ${MOC_HDRS})

include(${QT_USE_FILE})

# Main executable
add_executable(launcher
  ${LAUNCHER}
  ${MISC} ${MISC_HEADER}
  ${FILES} ${FILES_HEADER}
  ${TO_UTF8}
  ${ESM}
  ${RCC_SRCS}
  ${MOC_SRCS}
)

target_link_libraries(launcher
  ${Boost_LIBRARIES}
  ${OGRE_LIBRARIES}
  ${QT_LIBRARIES}
)

configure_file(${CMAKE_SOURCE_DIR}/files/launcher.qss
    "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/launcher.qss")