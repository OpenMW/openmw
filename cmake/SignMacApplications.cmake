# This script re-signs OpenMW.app and OpenMW-CS.app after CPack packages them. This is necessary because CPack modifies
# the library references used by OpenMW to App relative paths, invalidating the code signature.

# Obviously, we only need to run this on Apple targets.
if (APPLE)
    set(OPENMW_APP "OpenMW")
    set(OPENMW_CS_APP "OpenMW-CS")

    set(APPLICATIONS "${OPENMW_APP}" "${OPENMW_CS_APP}")
    foreach(app_name IN LISTS APPLICATIONS)
        set(FULL_APP_PATH "${CPACK_TEMPORARY_INSTALL_DIRECTORY}/ALL_IN_ONE/${app_name}.app")
        message(STATUS "Re-signing ${app_name}.app")
        # Apple's codesign utility does not like directories with periods (.) in their names, so we'll remove it and
        # create a symlink using the original name, which codesign is fine with.
        file(GLOB OSG_PLUGINS_DIR "${FULL_APP_PATH}/Contents/PlugIns/osgPlugins*")
        file(RENAME "${OSG_PLUGINS_DIR}" "${FULL_APP_PATH}/Contents/PlugIns/osgPlugins")
        execute_process(COMMAND "ln" "-s" "osgPlugins" "${OSG_PLUGINS_DIR}"
                        WORKING_DIRECTORY "${FULL_APP_PATH}/Contents/PlugIns/")
        execute_process(COMMAND "codesign" "--force" "--deep" "-s" "-" "${FULL_APP_PATH}")
    endforeach(app_name)
endif (APPLE)