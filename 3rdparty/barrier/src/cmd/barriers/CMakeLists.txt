# barrier -- mouse and keyboard sharing utility
# Copyright (C) 2012-2016 Symless Ltd.
# Copyright (C) 2009 Nick Bolton
#
# This package is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# found in the file LICENSE that should have accompanied this file.
#
# This package is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

set(sources
    barriers.cpp
)

if (WIN32)
    file(GLOB arch_headers "MSWindows*.h")
    file(GLOB arch_sources "MSWindows*.cpp")
    list(APPEND sources
        barriers.rc
        barriers.exe.manifest)
elseif (APPLE)
    file(GLOB arch_headers "OSX*.h")
    file(GLOB arch_sources "OSX*.cpp")
elseif (UNIX)
    file(GLOB arch_headers "XWindows*.h")
    file(GLOB arch_sources "XWindows*.cpp")
endif()

list(APPEND sources ${arch_sources})
list(APPEND headers ${arch_headers})

if (BARRIER_ADD_HEADERS)
    list(APPEND sources ${headers})
endif()

add_executable(barriers ${sources})
target_link_libraries(barriers
    arch base client common io mt net ipc platform server synlib ${libs} ${OPENSSL_LIBS})

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    install (TARGETS barriers DESTINATION ${BARRIER_BUNDLE_BINARY_DIR})
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    if (DEFINED EXECUTABLE_INSTALL_DIR)
        install (TARGETS barriers DESTINATION ${EXECUTABLE_INSTALL_DIR})
    else()
        install (TARGETS barriers DESTINATION bin)
    endif()
endif()
