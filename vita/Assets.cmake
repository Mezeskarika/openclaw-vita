# Builds ASSETS.ZIP (menus, actor prototypes, level metadata, ...) from Build_Release/ASSETS/,
# so the VPK always carries what is in that folder. Sets ASSETS_ZIP to the generated file.
#
# Uses vita/tools/make_assets_zip.py rather than `cmake -E tar --format=zip`: the game's zip
# reader needs the sizes in each local header, which CMake's zip writer leaves at zero.

find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(ASSETS_DIR "${CMAKE_SOURCE_DIR}/Build_Release/ASSETS")
set(ASSETS_ZIP "${CMAKE_BINARY_DIR}/ASSETS.ZIP")

file(GLOB_RECURSE ASSETS_FILES CONFIGURE_DEPENDS "${ASSETS_DIR}/*")

add_custom_command(OUTPUT "${ASSETS_ZIP}"
    COMMAND ${Python3_EXECUTABLE} -I "${CMAKE_SOURCE_DIR}/vita/tools/make_assets_zip.py" "${ASSETS_DIR}" "${ASSETS_ZIP}"
    DEPENDS ${ASSETS_FILES} "${CMAKE_SOURCE_DIR}/vita/tools/make_assets_zip.py"
    COMMENT "Packing ASSETS.ZIP"
    VERBATIM
)
add_custom_target(assets_zip ALL DEPENDS "${ASSETS_ZIP}")
