# PS Vita packaging: turns the openclaw ELF into eboot.bin and bundles it into a VPK.
# Included from the root CMakeLists.txt so the ELF sits in CMAKE_CURRENT_BINARY_DIR, as vita_create_self() expects.
include("${VITASDK}/share/vita.cmake" REQUIRED)

set(VITA_APP_NAME "OpenClaw")
set(VITA_TITLEID  "OCLW00001")
set(VITA_VERSION  "01.00")

vita_create_self(eboot.bin openclaw)

include(vita/Freepats.cmake)
include(vita/Assets.cmake)

# CLAW.REZ is not bundled - the user copies it to ux0:data/openclaw/
vita_create_vpk(openclaw.vpk ${VITA_TITLEID} eboot.bin
    VERSION ${VITA_VERSION}
    NAME ${VITA_APP_NAME}
    FILE vita/sce_sys/icon0.png sce_sys/icon0.png
    FILE vita/sce_sys/livearea/contents/bg.png sce_sys/livearea/contents/bg.png
    FILE vita/sce_sys/livearea/contents/startup.png sce_sys/livearea/contents/startup.png
    FILE vita/sce_sys/livearea/contents/template.xml sce_sys/livearea/contents/template.xml
    FILE vita/config.xml config.xml
    FILE Build_Release/SAVES.XML SAVES.XML
    FILE ${ASSETS_ZIP} ASSETS.ZIP
    FILE Build_Release/clacon.ttf clacon.ttf
    FILE Build_Release/console02.tga console02.tga
    FILE ${FREEPATS_VPK_FILES}
)
