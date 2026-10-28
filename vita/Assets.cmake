# Builds ASSETS.ZIP (menus, actor prototypes, level metadata, ...) from Build_Release/ASSETS/,
# so the VPK always carries what is in that folder. Sets ASSETS_ZIP to the generated file.
#
# Entries are named relative to ASSETS/ ("MENU.xml", "CONTROLS/touchscreen_img.png"), which is
# how the game looks them up.

set(ASSETS_DIR "${CMAKE_SOURCE_DIR}/Build_Release/ASSETS")
set(ASSETS_ZIP "${CMAKE_BINARY_DIR}/ASSETS.ZIP")

file(GLOB ASSETS_TOP_LEVEL RELATIVE "${ASSETS_DIR}" "${ASSETS_DIR}/*")
file(GLOB_RECURSE ASSETS_FILES CONFIGURE_DEPENDS "${ASSETS_DIR}/*")

add_custom_command(OUTPUT "${ASSETS_ZIP}"
    COMMAND ${CMAKE_COMMAND} -E remove -f "${ASSETS_ZIP}"
    COMMAND ${CMAKE_COMMAND} -E tar cf "${ASSETS_ZIP}" --format=zip ${ASSETS_TOP_LEVEL}
    WORKING_DIRECTORY "${ASSETS_DIR}"
    DEPENDS ${ASSETS_FILES}
    COMMENT "Packing ASSETS.ZIP"
    VERBATIM
)
add_custom_target(assets_zip ALL DEPENDS "${ASSETS_ZIP}")
