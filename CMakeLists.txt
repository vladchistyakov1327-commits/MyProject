cmake_minimum_required(VERSION 3.20)

project(TrafficMonitor
    VERSION     3.0.0
    DESCRIPTION "Wi-Fi Traffic Monitor v3.0"
    LANGUAGES   CXX RC
)

# ============================================================
# Global settings
# ============================================================
set(CMAKE_CXX_STANDARD          20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS        OFF)

# Output to bin/ directory (for single-config generators: Ninja, Makefiles)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
# For MSVC multi-config generators (Visual Studio) — prevent config subfolder (bin/Release/ -> bin/)
foreach(_cfg Release Debug RelWithDebInfo MinSizeRel)
    set("CMAKE_RUNTIME_OUTPUT_DIRECTORY_${_cfg}" "${CMAKE_SOURCE_DIR}/bin")
endforeach()

# ============================================================
# Source files
# ============================================================
set(CORE_SOURCES
    src/core/data_collector.cpp
    src/core/statistics.cpp
    src/core/persistence.cpp
    src/core/traffic_monitor.cpp
)

set(UI_SOURCES
    src/ui/overlay_window.cpp
    src/ui/stats_window.cpp
    src/ui/settings_window.cpp
    src/ui/graph_painter.cpp
)

set(UTILS_SOURCES
    src/utils/logger.cpp
)

set(ALL_SOURCES
    src/main.cpp
    ${CORE_SOURCES}
    ${UI_SOURCES}
    ${UTILS_SOURCES}
    resources/resources.rc
)

# ============================================================
# Executable
# ============================================================
add_executable(TrafficMonitor WIN32 ${ALL_SOURCES})

# ============================================================
# Include directories
# ============================================================
target_include_directories(TrafficMonitor PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_SOURCE_DIR}/resources"
)

# ============================================================
# Compiler-specific settings
# ============================================================
if (MSVC)
    target_compile_options(TrafficMonitor PRIVATE
        /W4             # High warning level
        /WX-            # Warnings are not errors (set to /WX to treat as errors)
        /permissive-    # Strict conformance
        /Zc:__cplusplus # Correct __cplusplus macro
        /utf-8          # Source and execution charset as UTF-8
        /wd4100         # Suppress: unreferenced formal parameter
        /wd4996         # Suppress: deprecated functions
    )
    target_compile_definitions(TrafficMonitor PRIVATE
        _UNICODE
        UNICODE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        _WIN32_WINNT=0x0A00   # Windows 10+
        NTDDI_VERSION=0x0A000000
    )
    # Embed manifest
    target_sources(TrafficMonitor PRIVATE resources/app.manifest)

elseif (MINGW OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_compile_options(TrafficMonitor PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wno-unused-parameter
        -municode           # Unicode entry point
    )
    target_compile_definitions(TrafficMonitor PRIVATE
        _UNICODE
        UNICODE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        _WIN32_WINNT=0x0A00
        NTDDI_VERSION=0x0A000000
    )
    target_link_options(TrafficMonitor PRIVATE -municode)
endif()

# ============================================================
# Link libraries
# ============================================================
target_link_libraries(TrafficMonitor PRIVATE
    # Windows SDK libraries
    kernel32
    user32
    gdi32
    shell32
    comctl32
    comdlg32
    advapi32    # Registry
    iphlpapi    # Network stats (GetIfTable2)
    shlwapi     # Shell utilities
    uxtheme     # Visual themes
)

# ============================================================
# Installation
# ============================================================
install(TARGETS TrafficMonitor
    RUNTIME DESTINATION bin
)

# ============================================================
# Summary
# ============================================================
message(STATUS "===========================================")
message(STATUS "  Traffic Monitor v${PROJECT_VERSION}")
message(STATUS "  Compiler   : ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "  Standard   : C++${CMAKE_CXX_STANDARD}")
message(STATUS "  Build type : ${CMAKE_BUILD_TYPE}")
message(STATUS "  Output dir : ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "===========================================")
