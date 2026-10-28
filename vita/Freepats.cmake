# Fetches the FreePats instrument patches that vita/timidity.cfg refers to, for the music.
# Sets FREEPATS_VPK_FILES to "source destination" pairs for vita_create_vpk().
#
# The archive is the original FreePats 20060219 release as packaged by Debian
# (the upstream tarball is nested inside it). It is downloaded once into the build tree.

set(FREEPATS_URL "https://deb.debian.org/debian/pool/main/f/freepats/freepats_20060219.orig.tar.gz")
set(FREEPATS_SHA256 "70bf8ca084df3903d6c9de43fe20539fc0a553d95cfba4d525da3fe66fda5f10")

set(FREEPATS_ARCHIVE "${CMAKE_BINARY_DIR}/freepats_20060219.orig.tar.gz")
set(FREEPATS_DIR "${CMAKE_BINARY_DIR}/freepats-src/freepats")

if (NOT EXISTS "${FREEPATS_DIR}/COPYING")
    file(DOWNLOAD "${FREEPATS_URL}" "${FREEPATS_ARCHIVE}"
        EXPECTED_HASH SHA256=${FREEPATS_SHA256}
        STATUS FREEPATS_DOWNLOAD_STATUS)
    list(GET FREEPATS_DOWNLOAD_STATUS 0 FREEPATS_DOWNLOAD_ERROR)
    if (FREEPATS_DOWNLOAD_ERROR)
        message(FATAL_ERROR "Downloading FreePats failed: ${FREEPATS_DOWNLOAD_STATUS}")
    endif ()

    file(ARCHIVE_EXTRACT INPUT "${FREEPATS_ARCHIVE}" DESTINATION "${CMAKE_BINARY_DIR}/freepats-orig")
    file(ARCHIVE_EXTRACT
        INPUT "${CMAKE_BINARY_DIR}/freepats-orig/freepats-20060219/upstream/freepats-20060219.tar.bz2"
        DESTINATION "${CMAKE_BINARY_DIR}/freepats-src")
endif ()

set(FREEPATS_VPK_FILES
    vita/timidity.cfg freepats/timidity.cfg
    "${FREEPATS_DIR}/COPYING" freepats/COPYING
    "${FREEPATS_DIR}/README" freepats/README
)

# Ship only the patches the config refers to (several programs share one patch)
file(STRINGS "${CMAKE_SOURCE_DIR}/vita/timidity.cfg" FREEPATS_CFG_LINES REGEX "\\.pat")
set(FREEPATS_PATCHES "")
foreach (FREEPATS_LINE IN LISTS FREEPATS_CFG_LINES)
    string(REGEX MATCH "[A-Za-z]+_[0-9]+/[^ \t]+\\.pat" FREEPATS_PATCH "${FREEPATS_LINE}")
    if (FREEPATS_PATCH)
        list(APPEND FREEPATS_PATCHES "${FREEPATS_PATCH}")
    endif ()
endforeach ()
list(REMOVE_DUPLICATES FREEPATS_PATCHES)

foreach (FREEPATS_PATCH IN LISTS FREEPATS_PATCHES)
    if (NOT EXISTS "${FREEPATS_DIR}/${FREEPATS_PATCH}")
        message(FATAL_ERROR "vita/timidity.cfg refers to ${FREEPATS_PATCH}, which FreePats doesn't have")
    endif ()
    list(APPEND FREEPATS_VPK_FILES "${FREEPATS_DIR}/${FREEPATS_PATCH}" "freepats/${FREEPATS_PATCH}")
endforeach ()
