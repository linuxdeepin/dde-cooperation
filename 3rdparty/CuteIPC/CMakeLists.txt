cmake_minimum_required(VERSION 3.13)

# Project
PROJECT(CuteIPC VERSION 0.1.0)

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

SET(sources
  src/CuteIPCService.cpp
  src/CuteIPCInterface.cpp
  src/CuteIPCMarshaller.cpp
  src/CuteIPCServiceConnection.cpp
  src/CuteIPCInterfaceConnection.cpp
  src/CuteIPCMessage.cpp
  src/CuteIPCSignalHandler.cpp
  src/CuteIPCInterfaceWorker.cpp
)

SET(headers
  include/CuteIPCGlobal.h
  include/CuteIPCService.h
  include/CuteIPCInterface.h
  src/CuteIPCService_p.h
  src/CuteIPCInterface_p.h
  src/CuteIPCMarshaller_p.h
  src/CuteIPCMessage_p.h
  src/CuteIPCSignalHandler_p.h
)

SET(moc_headers
  src/CuteIPCServiceConnection_p.h
  src/CuteIPCInterfaceConnection_p.h
  src/CuteIPCInterfaceWorker.h
)

SET(moc_service
  ${CMAKE_CURRENT_BINARY_DIR}/moc_CuteIPCService.cpp
)

SET(moc_interface
  ${CMAKE_CURRENT_BINARY_DIR}/moc_CuteIPCInterface.cpp
)

IF (QT_DESIRED_VERSION MATCHES 6)
  # find_package(Qt${QT_DESIRED_VERSION} REQUIRED COMPONENTS Core5Compat)
  QT6_WRAP_CPP(sources ${moc_headers})
  QT6_GENERATE_MOC(include/CuteIPCService.h ${moc_service})
  QT6_GENERATE_MOC(include/CuteIPCInterface.h ${moc_interface})
ELSEIF (QT_DESIRED_VERSION MATCHES 5)
  QT5_WRAP_CPP(sources ${moc_headers})
  QT5_GENERATE_MOC(include/CuteIPCService.h ${moc_service})
  QT5_GENERATE_MOC(include/CuteIPCInterface.h ${moc_interface})
ELSE()
  QT4_WRAP_CPP(sources ${moc_headers})
  QT4_GENERATE_MOC(include/CuteIPCService.h ${moc_service})
  QT4_GENERATE_MOC(include/CuteIPCInterface.h ${moc_interface})
ENDIF()

SET_SOURCE_FILES_PROPERTIES(src/CuteIPCService.cpp PROPERTIES OBJECT_DEPENDS ${moc_service})
SET_SOURCE_FILES_PROPERTIES(src/CuteIPCInterface.cpp PROPERTIES OBJECT_DEPENDS ${moc_interface})

add_library(${PROJECT_NAME} ${sources} ${headers} ${moc_headers})

if(BUILD_SHARED_LIBS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE QT_BUILD_SHARED)
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

SET(CUTEIPC_BUILD_EXAMPLES OFF CACHE BOOL "Build CuteIPC examples")
SET(CUTEIPC_BUILD_TESTS OFF CACHE BOOL "Build CuteIPC tests")


# Examples
IF (CUTEIPC_BUILD_EXAMPLES)
  ADD_SUBDIRECTORY(example/client)
  ADD_SUBDIRECTORY(example/server)
ENDIF ()

IF (CUTEIPC_BUILD_TESTS)
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
