cmake_minimum_required(VERSION 3.2)

# Two builds: the PS Vita (-DVita=ON, normally through vita/build.sh) and a Linux host build
# used for profiling and debugging (see CLAUDE.md, "Debugging on the host").
option(Vita "Build for PS Vita (needs VITASDK)" OFF)
if (Vita AND NOT CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "VitaSDK toolchain file")
endif (Vita AND NOT CMAKE_TOOLCHAIN_FILE)

project(OpenClaw)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -g")

# CMake compiled directories with dependencies
add_subdirectory(Box2D)
add_subdirectory(libwap)
add_subdirectory(./ThirdParty/Tinyxml)

# Directories with headers
include_directories(./Box2D/)
include_directories(./libwap/)
if (Vita)
    # ThirdParty/SDL2 holds old SDL 2.0.4 headers, which would shadow the VitaSDK ones.
    # Expose only the other ThirdParty headers.
    file(COPY ./ThirdParty/Tinyxml ./ThirdParty/FastDelegate
        DESTINATION ${CMAKE_BINARY_DIR}/ThirdParty
        FILES_MATCHING PATTERN "*.h")
    include_directories(${CMAKE_BINARY_DIR}/ThirdParty/)
else (Vita)
    include_directories(./ThirdParty/)
endif (Vita)
include_directories(./ThirdParty/Tinyxml/)

if (Vita)
    # vita_create_self() expects the ELF next to the CMake binary dir
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
else (Vita)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ../Build_Release)
endif (Vita)

add_executable(openclaw "")

add_subdirectory(OpenClaw)

# Linker settings
list(APPEND TARGET_LIBS
    libwap
    tinyxml
    Box2D
)
if (Vita)
    # vita-elf-create appends ~4 KB of module data after the code, and fails ("segment 1 overlaps")
    # when the code happens to end just short of the next 64 KiB boundary. The SDK's linker
    # script moves the data segment out by __sce_headroom, so that space always exists.
    set_target_properties(openclaw PROPERTIES LINK_FLAGS "-Wl,--defsym=__sce_headroom=0x2000")

    # Everything is static on the Vita, so the order matters: SDL add-ons, their codecs,
    # SDL2 with its vitaGL backend, then the system stubs. The SDK's CMake configs and
    # pkg-config files both leave some of these out (zlib, libpng, SceShaccCgExt, pthread).
    list(APPEND TARGET_LIBS
            SDL2_mixer SDL2_ttf SDL2_image SDL2_gfx
            opusfile opus vorbisfile vorbis ogg modplug xmp-lite
            freetype png16 bz2 webpdemux webp sharpyuv z
            SDL2 vitaGL vitashark SceShaccCgExt mathneon taihen_stub
            SceShaccCg_stub SceKernelDmacMgr_stub SceIme_stub SceGxm_stub SceDisplay_stub
            SceCtrl_stub SceAppMgr_stub SceAppUtil_stub SceAudio_stub SceAudioIn_stub
            SceSysmodule_stub SceIofilemgr_stub SceCommonDialog_stub SceTouch_stub SceHid_stub
            SceMotion_stub ScePower_stub SceProcessmgr_stub
            stdc++
            pthread
            m
            )
else (Vita)
    list(APPEND TARGET_LIBS
            SDL2
            SDL2_mixer
            SDL2_ttf
            SDL2_image
            SDL2_gfx
            stdc++
            m
            )
endif (Vita)

target_link_libraries(openclaw ${TARGET_LIBS})

if (Vita)
    include(vita/Vita.cmake)
endif (Vita)
