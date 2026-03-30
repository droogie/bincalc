cmake_minimum_required(VERSION 3.16)

project(bincalc VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

include(GNUInstallDirs)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

add_executable(bincalc WIN32 MACOSX_BUNDLE
    main.cpp
    calculator_core.cpp
    calculator_core.h
    numeric_formats.cpp
    numeric_formats.h
    bincalc.cpp
    bincalc.h
    bincalc.ui
    resources.qrc
)

target_link_libraries(bincalc PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

set_target_properties(bincalc PROPERTIES
    MACOSX_BUNDLE_BUNDLE_NAME "BinCalc"
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.bincalc.app"
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
)

add_executable(bincalc_tests
    tests.cpp
    calculator_core.cpp
    calculator_core.h
    numeric_formats.cpp
    numeric_formats.h
)

target_link_libraries(bincalc_tests PRIVATE Qt${QT_VERSION_MAJOR}::Core)

enable_testing()
add_test(NAME bincalc_tests COMMAND bincalc_tests)

install(TARGETS bincalc
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

set(CPACK_PACKAGE_NAME "bincalc")
set(CPACK_PACKAGE_VENDOR "bincalc")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}")
if(WIN32)
    set(CPACK_GENERATOR "ZIP")
elseif(APPLE)
    set(CPACK_GENERATOR "DragNDrop")
else()
    set(CPACK_GENERATOR "TGZ")
endif()
include(CPack)
