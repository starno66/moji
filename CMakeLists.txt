cmake_minimum_required(VERSION 3.16)
project(moji LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOUIC_SEARCH_PATHS ui)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network WebEngineWidgets)

set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/gitmanager.cpp
    src/filewatcher.cpp
    src/chapterlistmodel.cpp
    src/commitmodel.cpp
    src/commitdetailwidget.cpp
    src/environmentsetupdialog.cpp
    src/aidialog.cpp
    src/memodialog.cpp
)

set(HEADERS
    include/mainwindow.h
    include/gitmanager.h
    include/filewatcher.h
    include/chapterlistmodel.h
    include/commitmodel.h
    include/commitdetailwidget.h
    include/environmentsetupdialog.h
    include/aidialog.h
    include/memodialog.h
)

qt_add_executable(moji
    ${SOURCES}
    ${HEADERS}
    ui/mainwindow.ui
    resources/resources.qrc
    app.rc
)

target_include_directories(moji PRIVATE include)

target_link_libraries(moji PRIVATE
    Qt6::Widgets
    Qt6::Network
    Qt6::WebEngineWidgets
)
