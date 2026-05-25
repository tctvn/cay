cmake_minimum_required(VERSION 3.20)
project(cay LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# Sources
# ---------------------------------------------------------------------------
set(SOURCES
    src/platform/windows/main.cpp
    src/core/CayData.cpp
    src/core/CayEngine.cpp
    src/platform/windows/InputInjector.cpp
    src/platform/windows/KeyboardHookManager.cpp
    src/platform/windows/no_crt.cpp
)

add_executable(cay WIN32 ${SOURCES})

target_include_directories(cay PRIVATE src/core src/platform/windows)

# ---------------------------------------------------------------------------
# Compiler + linker flags (MSVC) – small static exe, no dynamic CRT DLL
# ---------------------------------------------------------------------------
if(MSVC)
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
endif()

