cmake_minimum_required(VERSION 3.20)
project(cay LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Build for both Apple Silicon and Intel" FORCE)
    enable_language(OBJCXX)
endif()

# ---------------------------------------------------------------------------
# Core Sources
# ---------------------------------------------------------------------------
set(CORE_SOURCES
    src/core/CayData.cpp
    src/core/CayEngine.cpp
)

# ---------------------------------------------------------------------------
# OS-Specific setup
# ---------------------------------------------------------------------------
if(WIN32)
    set(PLATFORM_MINIMAL_SOURCES
        src/platform/windows/minimal/main.cpp
        src/platform/windows/minimal/no_crt.cpp
    )
    
    set(PLATFORM_SHARED_SOURCES
        src/platform/windows/shared/InputInjector.cpp
        src/platform/windows/shared/KeyboardHookManager.cpp
    )

    # -----------------------------------------------------------------------
    # Minimal Target (cay.exe) - Pure Native, No CRT
    # -----------------------------------------------------------------------
    add_executable(cay WIN32 ${CORE_SOURCES} ${PLATFORM_MINIMAL_SOURCES} ${PLATFORM_SHARED_SOURCES})
    target_include_directories(cay PRIVATE src/core src/platform/windows/shared)

    # Strip CMake-injected /EHsc and /GR so we can override them cleanly
    string(REPLACE "/EHsc" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    string(REPLACE "/GR"   "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

    target_compile_options(cay PRIVATE
        /W3 /O1 /Os /GS- /EHs-c- /GR- /Gy /MT /utf-8
        /DUNICODE /D_UNICODE /DWIN32 /D_WINDOWS /DNDEBUG
    )

    target_link_options(cay PRIVATE
        /NODEFAULTLIB:msvcrt.lib /NODEFAULTLIB:msvcrtd.lib
        /NODEFAULTLIB:libcmtd.lib /NODEFAULTLIB:vcruntimed.lib /NODEFAULTLIB:ucrtd.lib
        /ENTRY:wWinMain /SUBSYSTEM:WINDOWS /OPT:REF /OPT:ICF /MANIFEST:NO
    )

    target_link_libraries(cay PRIVATE
        kernel32.lib user32.lib gdi32.lib shell32.lib advapi32.lib
        libcmt.lib libvcruntime.lib libucrt.lib
    )

    # -----------------------------------------------------------------------
    # Full Target (cayy.exe) - Native Win32 with Resource Dialog (.rc)
    # -----------------------------------------------------------------------
    set(PLATFORM_FULL_SOURCES
        src/platform/windows/full/main_full.cpp
        src/platform/windows/full/ConfigManager.cpp
        src/platform/windows/full/cayy.rc
    )
    
    add_executable(cayy WIN32 ${CORE_SOURCES} ${PLATFORM_FULL_SOURCES} ${PLATFORM_SHARED_SOURCES})
    target_include_directories(cayy PRIVATE src/core src/platform/windows/shared src/platform/windows/full)
    
    target_compile_options(cayy PRIVATE
        /W3 /O1 /Os /GS- /EHs-c- /GR- /Gy /MT /utf-8
        /DUNICODE /D_UNICODE /DWIN32 /D_WINDOWS /DNDEBUG
    )
    
    target_link_options(cayy PRIVATE
        /NODEFAULTLIB:msvcrt.lib /NODEFAULTLIB:msvcrtd.lib
        /NODEFAULTLIB:libcmtd.lib /NODEFAULTLIB:vcruntimed.lib /NODEFAULTLIB:ucrtd.lib
        /SUBSYSTEM:WINDOWS /OPT:REF /OPT:ICF /MANIFEST:NO
    )
    
    target_link_libraries(cayy PRIVATE
        kernel32.lib user32.lib gdi32.lib shell32.lib advapi32.lib comctl32.lib
        libcmt.lib libvcruntime.lib libucrt.lib libcpmt.lib
    )


    # -----------------------------------------------------------------------
    # Classic Target (cay-classic.exe) - Legacy Unikey UI with Cay Engine
    # -----------------------------------------------------------------------
    set(PLATFORM_CLASSIC_UI_SOURCES
        src/platform/windows/classic/newkey/about.cpp
        src/platform/windows/classic/newkey/button.cpp
        src/platform/windows/classic/newkey/CayIntegration.cpp
        src/platform/windows/classic/newkey/encode.cpp
        src/platform/windows/classic/newkey/expdlg.cpp
        src/platform/windows/classic/newkey/font.cpp
        src/platform/windows/classic/newkey/keydlg.cpp
        src/platform/windows/classic/newkey/label.cpp
        src/platform/windows/classic/newkey/macrodlg.cpp
        src/platform/windows/classic/newkey/mactab.cpp
        src/platform/windows/classic/newkey/mainwnd.cpp
        src/platform/windows/classic/newkey/odmenu.cpp
        src/platform/windows/classic/newkey/prehdr.cpp
        src/platform/windows/classic/newkey/smedit.cpp
        src/platform/windows/classic/newkey/tooldlg.cpp
        src/platform/windows/classic/newkey/ukmenu.cpp
        src/platform/windows/classic/newkey/userpref.cpp
        src/platform/windows/classic/newkey/util.cpp
        src/platform/windows/classic/newkey/NewKey.rc
    )
    
    set(PLATFORM_CLASSIC_CONV_SOURCES
        src/platform/windows/classic/vnconv/charset.cpp
        src/platform/windows/classic/vnconv/convert.cpp
        src/platform/windows/classic/vnconv/data.cpp
        src/platform/windows/classic/vnconv/error.cpp
        src/platform/windows/classic/vnconv/pattern.cpp
        src/platform/windows/classic/vnconv/stdafx.cpp
        src/platform/windows/classic/vnconv/docxio.cpp
        src/platform/windows/classic/vnconv/miniz.cpp
        src/platform/windows/classic/vnconv/miniz_tdef.cpp
        src/platform/windows/classic/vnconv/miniz_tinfl.cpp
        src/platform/windows/classic/vnconv/miniz_zip.cpp
        src/platform/windows/classic/byteio/byteio.cpp
    )
    
    add_executable(cay-classic WIN32 ${CORE_SOURCES} ${PLATFORM_CLASSIC_UI_SOURCES} ${PLATFORM_CLASSIC_CONV_SOURCES} ${PLATFORM_SHARED_SOURCES})
    set_source_files_properties(${PLATFORM_CLASSIC_UI_SOURCES} PROPERTIES COMPILE_OPTIONS "/utf-8")
    target_include_directories(cay-classic PRIVATE 
        src/core 
        src/platform/windows/shared 
        src/platform/windows/classic/newkey 
        src/platform/windows/classic/vnconv
        src/platform/windows/classic/byteio
    )
    
    target_compile_definitions(cay-classic PRIVATE _UNICODE UNICODE WIN32 _WINDOWS NDEBUG)
    
    target_compile_options(cay-classic PRIVATE
        /W3 /EHsc /J /Zc:forScope-
    )
    
    target_link_options(cay-classic PRIVATE
        /SUBSYSTEM:WINDOWS
    )
    
    target_link_libraries(cay-classic PRIVATE
        kernel32.lib user32.lib gdi32.lib shell32.lib advapi32.lib comctl32.lib comdlg32.lib
    )

elseif(APPLE)
    set(PLATFORM_SOURCES
        src/platform/macos/AppDelegate.mm
        src/platform/macos/MacInputInjector.mm
        src/platform/macos/MacHookManager.mm
    )

    set_source_files_properties(
        ${PLATFORM_SOURCES}
        PROPERTIES COMPILE_FLAGS "-fobjc-arc"
    )
    
    # MACOSX_BUNDLE makes it an .app bundle
    add_executable(cay MACOSX_BUNDLE ${CORE_SOURCES} ${PLATFORM_SOURCES})
    target_include_directories(cay PRIVATE src/core src/platform/macos)

    # Bundle configuration
    set_target_properties(cay PROPERTIES
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.tctvn.cay"
        MACOSX_BUNDLE_BUNDLE_NAME "Cay"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0.5"
        MACOSX_BUNDLE_BUNDLE_VERSION "1"
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/macos/Info.plist.in
    )

    # Link frameworks
    find_library(COCOA_FRAMEWORK Cocoa)
    find_library(COREGRAPHICS_FRAMEWORK CoreGraphics)
    find_library(CARBON_FRAMEWORK Carbon)
    find_library(SERVICEMANAGEMENT_FRAMEWORK ServiceManagement)
    
    target_link_libraries(cay PRIVATE
        ${COCOA_FRAMEWORK}
        ${COREGRAPHICS_FRAMEWORK}
        ${CARBON_FRAMEWORK}
        ${SERVICEMANAGEMENT_FRAMEWORK}
    )
    
    # Compile options for minimal size on macOS
    target_compile_options(cay PRIVATE -Os -fno-exceptions -fno-rtti)

elseif(CMAKE_SYSTEM_NAME MATCHES "Linux")
    # Old uinput code was removed by the user. Fcitx5 is now the primary Linux target.
endif()

# ---------------------------------------------------------------------------
# Fcitx5 Plugin (Optional on Linux)
# ---------------------------------------------------------------------------
option(BUILD_FCITX5 "Build Fcitx5 Plugin" OFF)
if(BUILD_FCITX5)
    add_subdirectory(src/platform/fcitx5)
endif()
