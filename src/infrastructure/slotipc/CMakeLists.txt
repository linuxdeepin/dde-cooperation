cmake_minimum_required(VERSION 3.13)

# Project
PROJECT(slotipc VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

ENABLE_TESTING(true)

# CMake module path
SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/CMake)

# Compile flags
INCLUDE(CompilerFlags)

# Qt
IF (DEFINED QT_DESIRED_VERSION)
  MESSAGE("Build with setting Qt version!")
ELSE (DEFINED QT_DESIRED_VERSION)
  MESSAGE("Searching for preferred Qt version...")
  find_package(QT NAMES Qt6 Qt5 Qt4 REQUIRED COMPONENTS Core)
  SET(QT_DESIRED_VERSION ${QT_VERSION_MAJOR})
ENDIF (DEFINED QT_DESIRED_VERSION)

MESSAGE("Qt version is used: ${QT_DESIRED_VERSION}")
find_package(Qt${QT_DESIRED_VERSION} REQUIRED COMPONENTS Core Network Gui)

IF (QT_DESIRED_VERSION MATCHES 6)
  find_package(Qt${QT_DESIRED_VERSION} COMPONENTS Core5Compat REQUIRED)
ENDIF()

# Include directories
INCLUDE_DIRECTORIES(include src ${CMAKE_CURRENT_BINARY_DIR})

# Turn off automoc
SET(CMAKE_AUTOMOC OFF)

set(PUB_HEADER_SERVICE "include/slotipc/service.h")
set(PUB_HEADER_INTERFACE "include/slotipc/interface.h")

SET(sources
  src/service.cpp
  src/interface.cpp
  src/marshaller.cpp
  src/serviceconnection.cpp
  src/interfaceconnection.cpp
  src/message.cpp
  src/signalhandler.cpp
  src/interfaceworker.cpp
)

SET(headers
  include/slotipc/argdefine.h
  ${PUB_HEADER_SERVICE}
  ${PUB_HEADER_INTERFACE}
  src/service_p.h
  src/interface_p.h
  src/marshaller_p.h
  src/message_p.h
  src/signalhandler_p.h
)

SET(moc_headers
  src/serviceconnection_p.h
  src/interfaceconnection_p.h
  src/interfaceworker.h
)

SET(moc_service
  ${CMAKE_CURRENT_BINARY_DIR}/moc_service.cpp
)

SET(moc_interface
  ${CMAKE_CURRENT_BINARY_DIR}/moc_interface.cpp
)

IF (QT_DESIRED_VERSION MATCHES 6)
  # find_package(Qt${QT_DESIRED_VERSION} REQUIRED COMPONENTS Core5Compat)
  QT6_WRAP_CPP(sources ${moc_headers})
  QT6_GENERATE_MOC(${PUB_HEADER_SERVICE} ${moc_service})
  QT6_GENERATE_MOC(${PUB_HEADER_INTERFACE} ${moc_interface})
ELSEIF (QT_DESIRED_VERSION MATCHES 5)
  QT5_WRAP_CPP(sources ${moc_headers})
  QT5_GENERATE_MOC(${PUB_HEADER_SERVICE} ${moc_service})
  QT5_GENERATE_MOC(${PUB_HEADER_INTERFACE} ${moc_interface})
ELSE()
  QT4_WRAP_CPP(sources ${moc_headers})
  QT4_GENERATE_MOC(${PUB_HEADER_SERVICE} ${moc_service})
  QT4_GENERATE_MOC(${PUB_HEADER_INTERFACE} ${moc_interface})
ENDIF()

SET_SOURCE_FILES_PROPERTIES(src/service.cpp PROPERTIES OBJECT_DEPENDS ${moc_service})
SET_SOURCE_FILES_PROPERTIES(src/interface.cpp PROPERTIES OBJECT_DEPENDS ${moc_interface})

add_library(${PROJECT_NAME} ${sources} ${headers} ${moc_headers})

if(BUILD_SHARED_LIBS)
    set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION 0)
    set_target_properties(${PROJECT_NAME} PROPERTIES CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN ON)
endif()

if(FPIC)
    set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    Qt${QT_DESIRED_VERSION}::Core
    Qt${QT_DESIRED_VERSION}::Network
    Qt${QT_DESIRED_VERSION}::Gui
)

IF (QT_DESIRED_VERSION MATCHES 6)
    target_link_libraries(${PROJECT_NAME}
        PRIVATE
        Qt${QT_DESIRED_VERSION}::Core5Compat
    )
ENDIF()

TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME} PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

SET(SLOTIPC_BUILD_EXAMPLES OFF CACHE BOOL "Build SlotIPC examples")
SET(SLOTIPC_BUILD_TESTS OFF CACHE BOOL "Build SlotIPC tests")


# Examples
IF (SLOTIPC_BUILD_EXAMPLES)
  ADD_SUBDIRECTORY(example/client)
  ADD_SUBDIRECTORY(example/server)
ENDIF ()

IF (SLOTIPC_BUILD_TESTS)
  ADD_SUBDIRECTORY(test)
ENDIF ()

#INSTALL(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
if(WIN32)
    INSTALL(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
else()
    INSTALL(TARGETS ${PROJECT_NAME}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
endif()
