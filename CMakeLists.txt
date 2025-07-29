cmake_minimum_required(VERSION 3.19)
project(melody LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Network Multimedia)

qt_standard_project_setup()

set(CMAKE_AUTORCC ON)

qt_add_executable(melody
    WIN32 MACOSX_BUNDLE
    main.cpp
    resources.qrc
    widget.cpp
    widget.h
    apimanager.cpp
    apimanager.h
    playlistmanager.cpp
    playlistmanager.h
)

target_link_libraries(melody
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt::Network
        Qt::Multimedia
)

include(GNUInstallDirs)

install(TARGETS melody
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET melody
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
