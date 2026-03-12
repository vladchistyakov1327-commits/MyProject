cmake_minimum_required(VERSION 3.25)
project(NestingApp VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Concurrent)
find_package(ZLIB REQUIRED)

include(FetchContent)

# ── Clipper2 ──────────────────────────────────────────────────────────────────
FetchContent_Declare(clipper2
  GIT_REPOSITORY https://github.com/AngusJohnson/Clipper2.git
  GIT_TAG        main
  SOURCE_SUBDIR  CPP
)
FetchContent_MakeAvailable(clipper2)

# ── libdxfrw ──────────────────────────────────────────────────────────────────
set(BUILD_SHARED_LIBS_SAVED ${BUILD_SHARED_LIBS})
set(BUILD_SHARED_LIBS OFF)
FetchContent_Declare(libdxfrw
  GIT_REPOSITORY https://github.com/LibreCAD/libdxfrw.git
  GIT_TAG        master
)
FetchContent_MakeAvailable(libdxfrw)
set(BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS_SAVED})

# Убедиться, что заголовки libdxfrw доступны как interface
if(TARGET dxfrw AND NOT TARGET libdxfrw::dxfrw)
    target_include_directories(dxfrw PUBLIC
        $<BUILD_INTERFACE:${libdxfrw_SOURCE_DIR}/src>
    )
endif()

# ── Catch2 (тесты) ────────────────────────────────────────────────────────────
FetchContent_Declare(Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.4.0
)
FetchContent_MakeAvailable(Catch2)

# ── Core sources ──────────────────────────────────────────────────────────────
set(CORE_SOURCES
    core/logging/AppLogger.cpp
    core/geometry/PolyContour.cpp
    core/geometry/GeomUtils.cpp
    core/geometry/DxfImporter.cpp
    core/geometry/CoedgeDetector.cpp
    core/nesting/RotationSampler.cpp
    core/nesting/NFPCalculator.cpp
    core/nesting/SheetManager.cpp
    core/nesting/NestEngine.cpp
    core/export/LxdsExporter.cpp
    core/export/DxfExporter.cpp
    core/export/ReportGenerator.cpp
)

# ── UI sources ────────────────────────────────────────────────────────────────
set(UI_SOURCES
    AppStyle.cpp
    ui/TopBar.cpp
    ui/MainWindow.cpp
    ui/tabs/PartListTab.cpp
    ui/tabs/NestSettingsTab.cpp
    ui/tabs/SheetsTab.cpp
    ui/tabs/LogTab.cpp
    ui/canvas/NestCanvas.cpp
    ui/canvas/PartGraphicsItem.cpp
    ui/canvas/SheetGraphicsItem.cpp
    ui/dialogs/ImportDxfDialog.cpp
    ui/dialogs/SheetSetupDialog.cpp
    ui/dialogs/ExportDialog.cpp
    ui/widgets/ProgressOverlay.cpp
    ui/widgets/UtilizationBar.cpp
    ui/widgets/LogWidget.cpp
)

# ── Main executable ───────────────────────────────────────────────────────────
add_executable(NestingApp
    main.cpp
    ${CORE_SOURCES}
    ${UI_SOURCES}
)

target_include_directories(NestingApp PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${CMAKE_CURRENT_SOURCE_DIR}/ui
    ${libdxfrw_SOURCE_DIR}/src
)

target_link_libraries(NestingApp PRIVATE
    Qt6::Widgets
    Qt6::Concurrent
    Clipper2
    dxfrw
    ZLIB::ZLIB
)

# ── Tests ─────────────────────────────────────────────────────────────────────
enable_testing()

add_executable(NestingTests
    tests/main.cpp
    tests/test_dxf_importer.cpp
    tests/test_nfp_calculator.cpp
    tests/test_nest_engine.cpp
    tests/test_lxds_exporter.cpp
    ${CORE_SOURCES}
)

target_include_directories(NestingTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${libdxfrw_SOURCE_DIR}/src
)

target_link_libraries(NestingTests PRIVATE
    Catch2::Catch2
    Qt6::Widgets
    Qt6::Concurrent
    Clipper2
    dxfrw
    ZLIB::ZLIB
)

include(CTest)
include(Catch)
catch_discover_tests(NestingTests)
