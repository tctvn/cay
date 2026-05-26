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
    set(PLATFORM_SOURCES
        src/platform/windows/main.cpp
        src/platform/windows/InputInjector.cpp
        src/platform/windows/KeyboardHookManager.cpp
        src/platform/windows/no_crt.cpp
    )
    add_executable(cay WIN32 ${CORE_SOURCES} ${PLATFORM_SOURCES})
    target_include_directories(cay PRIVATE src/core src/platform/windows)

    # Strip CMake-injected /EHsc and /GR so we can override them cleanly
    string(REPLACE "/EHsc" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    string(REPLACE "/GR"   "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

    target_compile_options(cay PRIVATE
        /W3          # Warning level 3
        /O1          # Optimise for size
        /Os          # Favour small code
        /GS-         # Disable buffer security check
        /EHs-c-      # Disable C++ exceptions
        /GR-         # Disable RTTI
        /Gy          # Function-level linking (COMDAT)
        /MT          # Static CRT → tells headers to use non-import signatures
        /DUNICODE /D_UNICODE
        /DWIN32 /D_WINDOWS
        /DNDEBUG
    )

    target_link_options(cay PRIVATE
        # Exclude dynamic and debug CRT variants only
        /NODEFAULTLIB:msvcrt.lib
        /NODEFAULTLIB:msvcrtd.lib
        /NODEFAULTLIB:libcmtd.lib
        /NODEFAULTLIB:vcruntimed.lib
        /NODEFAULTLIB:ucrtd.lib
        /ENTRY:wWinMain          # Our GUI entry point
        /SUBSYSTEM:WINDOWS
        /OPT:REF                 # Strip unreferenced functions/data
        /OPT:ICF                 # Merge identical COMDATs
        /MANIFEST:NO             # No UAC manifest
        /machine:x64
    )

    # Win32 API + static CRT libraries (explicit list, no auto-link ambiguity)
    target_link_libraries(cay PRIVATE
        kernel32.lib
        user32.lib
        gdi32.lib
        shell32.lib
        advapi32.lib
        libcmt.lib        # Static CRT – wcslen, wcscpy_s, printf family
        libvcruntime.lib  # CRT helpers – __chkstk, memset, memcpy etc.
        libucrt.lib       # Universal CRT – wide-char, math etc.
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
        MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0.6"
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
