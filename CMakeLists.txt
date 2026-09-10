cmake_minimum_required(VERSION 3.20)
project(Mem-Map LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Qt 6 setup
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

# Optimization flags for GCC / Clang
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-O3 -Wall -Wextra -Wno-unused-parameter)
endif()

set(CORE_SOURCES
    src/core/DiskNode.h
    src/core/DiskNode.cpp
    src/core/ScannerEngine.h
    src/core/ScannerEngine.cpp
    src/core/ReportExporter.h
    src/core/ReportExporter.cpp
    src/core/SnapshotEngine.h
    src/core/SnapshotEngine.cpp
    src/core/DuplicateFinder.h
    src/core/DuplicateFinder.cpp
)

set(UI_SOURCES
    src/ui/TreemapLayout.h
    src/ui/TreemapLayout.cpp
    src/ui/TreemapWidget.h
    src/ui/TreemapWidget.cpp
    src/ui/SunburstWidget.h
    src/ui/SunburstWidget.cpp
    src/ui/DiskTreeModel.h
    src/ui/DiskTreeModel.cpp
    src/ui/SizeBarDelegate.h
    src/ui/SizeBarDelegate.cpp
    src/ui/ExtensionStatsWidget.h
    src/ui/ExtensionStatsWidget.cpp
    src/ui/TopFilesWidget.h
    src/ui/TopFilesWidget.cpp
    src/ui/BreadcrumbWidget.h
    src/ui/BreadcrumbWidget.cpp
    src/ui/SnapshotDiffWidget.h
    src/ui/SnapshotDiffWidget.cpp
    src/ui/DuplicateFilesWidget.h
    src/ui/DuplicateFilesWidget.cpp
    src/ui/MainWindow.h
    src/ui/MainWindow.cpp
)

set(PLATFORM_SOURCES "")
if(WIN32)
    set(PLATFORM_SOURCES resources/app.rc)
endif()

add_executable(Mem-Map
    src/main.cpp
    ${CORE_SOURCES}
    ${UI_SOURCES}
    ${PLATFORM_SOURCES}
)

target_include_directories(Mem-Map PRIVATE
    src
    src/core
    src/ui
)

target_link_libraries(Mem-Map PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

if(WIN32)
    target_link_libraries(Mem-Map PRIVATE
        shlwapi
        shell32
    )
endif()

# Automated test runner
add_executable(test_runner
    tests/test_main.cpp
    ${CORE_SOURCES}
    src/ui/TreemapLayout.h
    src/ui/TreemapLayout.cpp
)

target_include_directories(test_runner PRIVATE
    src
    src/core
    src/ui
)

target_link_libraries(test_runner PRIVATE
    Qt6::Core
    Qt6::Gui
)

if(WIN32)
    target_link_libraries(test_runner PRIVATE
        shlwapi
    )
endif()
