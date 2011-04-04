set(LAUNCHER
  datafilesdialog.cpp
  lineedit.cpp
  main.cpp
  maindialog.cpp
  settingsdialog.cpp
  datafilesdialog.h
  lineedit.h
  maindialog.h
  settingsdialog.h
)

SET(MOC_HDRS
  datafilesdialog.h
  lineedit.h
  maindialog.h
  settingsdialog.h
)

source_group(apps\\launcher FILES ${ESMTOOL})

find_package(Qt4 REQUIRED)
set(QT_USE_QTGUI 1)

QT4_ADD_RESOURCES(RCC_SRCS resources.qrc)
QT4_WRAP_CPP(MOC_SRCS ${MOC_HDRS})

include(${QT_USE_FILE})

# Main executable
add_executable(launcher
  ${LAUNCHER}
  ${MISC} ${MISC_HEADER}
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



#if (APPLE)
#    find_library(CARBON_FRAMEWORK Carbon)
#    target_link_libraries(openmw ${CARBON_FRAMEWORK})
#endif (APPLE)
