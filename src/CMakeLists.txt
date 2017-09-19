cmake_minimum_required(VERSION 3.6)

project(LLRPLapsSrc)

set(CMAKE_CONFIGURATION_TYPES Debug Release CACHE TYPE INTERNAL FORCE)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTORCC ON)

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
set(LIBXML2_INCLUDE_DIR "${TOP_PATH}/thirdparty/include/libxml2")
set(LIBXML2_LIBRARIES "${TOP_PATH}/thirdparty/lib/libxml2.lib")
set(LIBXSLT_INCLUDE_DIR "${TOP_PATH}/thirdparty/include/libxslt")
set(LIBXSLT_LIBRARIES "${TOP_PATH}/thirdparty/lib/libxslt.lib")
set(LIBXSLT_XSLTPROC_EXECUTABLE "${TOP_PATH}/thirdparty/bin/xsltproc.exe")
set(WINSOCK "Ws2_32.lib")
else()
find_package(LIBXML2 REQUIRED)
find_package(LIBXSLT REQUIRED)
set(WINSOCK "")
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	set(CMAKE_PREFIX_PATH "${QTDIR}/msvc2015_64/lib/cmake")
	set(EXE_OPTION "WIN32")
    set(LTKCPPLIB "ltkcpp.lib")
    set(LLRPLIB "llrplib.lib")
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(CMAKE_PREFIX_PATH "${QTDIR}/clang_64/lib/cmake")
    set(LTKCPPLIB "libltkcpp.dylib")
    set(LLRPLIB "libllrplib.dylib")
else()
    set(CMAKE_PREFIX_PATH "${QTDIR}/share")
    set(LTKCPPLIB "libltkcpp.so")
    set(LLRPLIB "libllrplib.so")
endif()

find_package(Qt5Widgets NO_MODULE REQUIRED)
find_package(Qt5LinguistTools NO_MODULE REQUIRED)

set(laps_SOURCES
        creader.cpp
        main.cpp
        mainwindow.cpp
        ctaginfo.cpp ctaginfo.h exceptions.cpp)
set(laps_HEADERS
        creader.h
        mainwindow.h
        exceptions.h)

set(laps_FORMS
        mainwindow.ui
        )

set(laps_TRANSLATIONS
        laps_fr_CA.ts
        )

QT5_WRAP_UI(laps_HEADERS_MOC ${laps_FORMS})
QT5_ADD_RESOURCES(laps_RESOURCES_RCC "resources/resources.qrc")

SET_DIRECTORY_PROPERTIES(PROPERTIES CLEAN_NO_CUSTOM TRUE)
# Generates the ts file from the sources, and then outputs a qm file in the build directory
# The variable laps_TRANSLATIONS_COMPILED holds the name of the qm file
QT5_CREATE_TRANSLATION (laps_TRANSLATIONS_COMPILED ${laps_TRANSLATIONS}
        ${laps_FORMS}
        ${laps_HEADERS}
        ${laps_SOURCES}
        ${laps_RESOURCES_RCC}
        )


include_directories(
        ${LIBXML2_INCLUDE_DIR}
        ${LIBXSLT_INCLUDE_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}
        ${LTKCPP_INCLUDE_PATH}
)

link_directories(
        ${CMAKE_CURRENT_BINARY_DIR}
        ${LTKCPP_LIB_PATH}
)

message("Library dir: ${LTKCPP_LIB_PATH}")

add_executable(laps ${EXE_OPTION}
        ${laps_SOURCES}
        ${laps_HEADERS_MOC}
        ${laps_HEADERS}
        ${laps_FORMS_HEADERS}
        ${laps_RESOURCES_RCC}
        ${laps_TRANSLATIONS_COMPILED})

set_target_properties(laps PROPERTIES OUTPUT_NAME laps) 
set_target_properties(laps PROPERTIES DEBUG_POSTFIX "d")
#set_target_properties(laps PROPERTIES VERSION "${LTKCPP_VERSION}")
#set_target_properties(lapsb PROPERTIES SOVERSION "${LTKCPP_VERSION}")

target_link_libraries(laps
	    ${LIBXML2_LIBRARIES}
	    ${LIBXSLT_LIBRARIES}
        ${LTKCPPLIB}
        ${LLRPLIB}
		${WINSOCK}
)

set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${CMAKE_CURRENT_BINARY_DIR}/laps_automoc.cpp" )

qt5_use_modules(laps Widgets Xml)

install(TARGETS laps
        RUNTIME DESTINATION ${INSTALL_BINDIR}
        LIBRARY DESTINATION ${INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${INSTALL_LIBDIR})

