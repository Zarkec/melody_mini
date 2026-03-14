cmake_minimum_required(VERSION 3.16)

project(melody VERSION 1.0.0 LANGUAGES CXX)

# -------------------------------------------------
# 基础 CMake / Qt 自动化配置
# -------------------------------------------------
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# -------------------------------------------------
# Qt（Qt6）
# -------------------------------------------------
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network Multimedia)

message(STATUS "Found Qt version: ${Qt6_VERSION}")

# -------------------------------------------------
# 源码组织
# -------------------------------------------------
set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)

set(CORE_SOURCES
    ${SRC_DIR}/core/apimanager.cpp
    ${SRC_DIR}/core/playlistmanager.cpp
)

set(CORE_HEADERS
    ${SRC_DIR}/core/apimanager.h
    ${SRC_DIR}/core/playlistmanager.h
)

set(UI_SOURCES
    ${SRC_DIR}/ui/widget.cpp
)

set(UI_HEADERS
    ${SRC_DIR}/ui/widget.h
)

set(MAIN_SOURCES
    ${SRC_DIR}/main.cpp
)

set(ALL_SOURCES
    ${CORE_SOURCES}
    ${CORE_HEADERS}
    ${UI_SOURCES}
    ${UI_HEADERS}
    ${MAIN_SOURCES}
)

# -------------------------------------------------
# 资源文件
# -------------------------------------------------
set(RESOURCES_DIR ${CMAKE_SOURCE_DIR}/resources)
set(RESOURCES
    ${RESOURCES_DIR}/resources.qrc
)

# -------------------------------------------------
# Windows 资源文件（用于设置 exe 图标）
# -------------------------------------------------
if(WIN32)
    set(WIN_RESOURCES ${RESOURCES_DIR}/app.rc)
endif()

# -------------------------------------------------
# 可执行目标
# -------------------------------------------------
qt_add_executable(melody
    WIN32 MACOSX_BUNDLE
    MANUAL_FINALIZATION
    ${ALL_SOURCES}
    ${RESOURCES}
    ${WIN_RESOURCES}
)

# -------------------------------------------------
# 头文件搜索路径
# -------------------------------------------------
target_include_directories(melody PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${SRC_DIR}
    ${SRC_DIR}/core
    ${SRC_DIR}/ui
    ${CMAKE_BINARY_DIR}
)

# -------------------------------------------------
# 链接库
# -------------------------------------------------
target_link_libraries(melody PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    Qt6::Multimedia
)

# -------------------------------------------------
# Qt6 finalize（必须）
# -------------------------------------------------
qt_finalize_executable(melody)

# -------------------------------------------------
# 安装 & windeployqt 自动部署（Windows）
# -------------------------------------------------
include(GNUInstallDirs)

install(TARGETS melody
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

if(WIN32)
    qt_generate_deploy_app_script(
        TARGET melody
        OUTPUT_SCRIPT deploy_script
        NO_UNSUPPORTED_PLATFORM_ERROR
    )
    install(SCRIPT ${deploy_script})
endif()
