cmake_minimum_required(VERSION 3.16)

project(MyMoneyBook VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)
find_package(Qt6 REQUIRED COMPONENTS Widgets Sql Charts)

set(PROJECT_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h

    databaseManager.cpp
    databaseManager.h

    LoginDialog.cpp
    LoginDialog.h

    billwidget.cpp
    billwidget.h

    categorymanagerwidget.cpp
    categorymanagerwidget.h

    budgetmanager.cpp
    budgetwidget.h

    searchfilterwidget.cpp
    searchfilterwidget.h

    visualizationwidget.cpp
    visualizationwidget.h

    accountmanagerwidget.cpp
    accountmanagerwidget.h
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(MyMoneyBook
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
else()
    if(ANDROID)
        add_library(MyMoneyBook SHARED
            ${PROJECT_SOURCES}
        )
    else()
        add_executable(MyMoneyBook
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(MyMoneyBook PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
target_link_libraries(MyMoneyBook PRIVATE Qt6::Widgets Qt6::Sql Qt6::Charts)

set_target_properties(MyMoneyBook PROPERTIES
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS MyMoneyBook
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(MyMoneyBook)
endif()
