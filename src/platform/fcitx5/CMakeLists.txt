find_package(Fcitx5Core REQUIRED)
include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(FCITX5_SOURCES
    CayimeEngine.cpp
    CayimeAddonFactory.cpp
)

# Core sources from parent
set(CORE_SOURCES
    ../../core/CayData.cpp
    ../../core/CayEngine.cpp
)

add_library(cayime-fcitx5 MODULE ${FCITX5_SOURCES} ${CORE_SOURCES})
target_include_directories(cayime-fcitx5 PRIVATE ../../core)
target_link_libraries(cayime-fcitx5 PRIVATE Fcitx5::Core Fcitx5::Config)

set_target_properties(cayime-fcitx5 PROPERTIES PREFIX "")

# Fcitx5 specific macro to translate conf
configure_file(cayime.conf.in.in cayime.conf @ONLY)

install(TARGETS cayime-fcitx5 DESTINATION "${CMAKE_INSTALL_LIBDIR}/fcitx5")
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/cayime.conf" DESTINATION "${CMAKE_INSTALL_DATADIR}/fcitx5/addon")
install(FILES "cayime-im.conf.in" DESTINATION "${CMAKE_INSTALL_DATADIR}/fcitx5/inputmethod" RENAME "cayime.conf")
install(FILES "cayime.svg" DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps")
