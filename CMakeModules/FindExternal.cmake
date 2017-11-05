#
#  CMake custom modules
#  Copyright (C) 2011-2015  Cedric OCHS
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

# Look for a directory containing external libraries.
#
# The following values are defined
# EXTERNAL_PATH         - where to find external
# EXTERNAL_INCLUDE_PATH - where to find external includes
# EXTERNAL_BINARY_PATH  - where to find external binaries
# EXTERNAL_LIBRARY_PATH - where to find external libraries
# EXTERNAL_FOUND        - True if the external libraries are available

IF(EXTERNAL_FOUND)
  RETURN()
ENDIF()

SET(EXTERNAL_NAME "external")

# List all supported compilers
SET(_COMPILERS MSVC60 MSVC70 MSVC71 MSVC80 MSVC90 MSVC10 MSVC11 MSVC12 MSVC13 MSVC14 MINGW CYGWIN ANDROID IOS OSX)

FOREACH(_COMPILER ${_COMPILERS})
  IF(${_COMPILER})
    SET(ENV_NAME "EXTERNAL_${_COMPILER}_PATH")
    SET(EXTERNAL_COMPILER_PATH $ENV{${ENV_NAME}})
  ENDIF()
ENDFOREACH()

IF(NOT EXTERNAL_PATH AND EXTERNAL_COMPILER_PATH)
  MESSAGE(STATUS "Using environment variable ${ENV_NAME}")
  SET(EXTERNAL_PATH ${EXTERNAL_COMPILER_PATH})
ENDIF()

IF(NOT EXTERNAL_PATH AND "$ENV{EXTERNAL_PATH}" MATCHES ".+")
  SET(EXTERNAL_PATH $ENV{EXTERNAL_PATH})
  MESSAGE(STATUS "Using environment variable EXTERNAL_PATH")
ENDIF()

IF(NOT EXTERNAL_PATH AND CMAKE_INSTALL_PREFIX)
  SET(EXTERNAL_PATH ${CMAKE_INSTALL_PREFIX})
  MESSAGE(STATUS "Using CMAKE_INSTALL_PREFIX")
ENDIF()

IF(NOT EXTERNAL_PATH)
  # Search in standard default pathes
  SET(EXTERNAL_TEMP_PATH ${CMAKE_CURRENT_SOURCE_DIR}/external ${CMAKE_CURRENT_SOURCE_DIR}/../external ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty ${CMAKE_CURRENT_SOURCE_DIR}/../3rdParty)
  SET(EXTERNAL_TEMP_FILE "include/zlib.h")

  # If using STLport preprend external_stlport
  IF(WITH_STLPORT)
    SET(EXTERNAL_TEMP_PATH ${CMAKE_CURRENT_SOURCE_DIR}/external_stlport ${CMAKE_CURRENT_SOURCE_DIR}/../external_stlport ${EXTERNAL_TEMP_PATH})
    SET(EXTERNAL_TEMP_FILE "include/stlport/string")
    SET(EXTERNAL_NAME "external with STLport")
  ENDIF()

  FIND_PATH(EXTERNAL_PATH
    HINTS
    ${EXTERNAL_TEMP_FILE}
    PATHS
    $ENV{EXTERNAL_PATH}
    ${EXTERNAL_TEMP_PATH}
    /usr/local
    /usr
    /sw
    /opt/local
    /opt/csw
    /opt
  )
ENDIF()

IF(EXTERNAL_PATH)
  SET(EXTERNAL_FOUND ON)

  # Fix path
  FILE(TO_CMAKE_PATH ${EXTERNAL_PATH} EXTERNAL_PATH)

  FOREACH(ITEM ${EXTERNAL_PATH})
    SET(EXTERNAL_INCLUDE_PATH ${EXTERNAL_INCLUDE_PATH} "${ITEM}/include" ${ITEM})

    SET(EXTERNAL_BINARY_PATH ${EXTERNAL_BINARY_PATH} "${ITEM}/bin${LIB_SUFFIX}" ${ITEM})
    SET(EXTERNAL_LIBRARY_PATH ${EXTERNAL_LIBRARY_PATH} "${ITEM}/lib${LIB_SUFFIX}")
    SET(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "${ITEM}/bin${LIB_SUFFIX}")
  ENDFOREACH()

  SET(CMAKE_PROGRAM_PATH ${EXTERNAL_BINARY_PATH} ${CMAKE_PROGRAM_PATH})
  SET(CMAKE_INCLUDE_PATH ${EXTERNAL_INCLUDE_PATH} ${CMAKE_INCLUDE_PATH})
  SET(CMAKE_LIBRARY_PATH ${EXTERNAL_LIBRARY_PATH} ${CMAKE_LIBRARY_PATH})
ENDIF()

IF(EXTERNAL_FOUND)
  # Special variables for boost
  IF(WIN32)
    SET(Boost_USE_STATIC_LIBS ON)
  ENDIF()

  IF(NOT External_FIND_QUIETLY)
    MESSAGE(STATUS "Found ${EXTERNAL_NAME}: ${EXTERNAL_PATH}")
  ENDIF()
ELSE()
  IF(External_FIND_REQUIRED)
    MESSAGE(FATAL_ERROR "Unable to find ${EXTERNAL_NAME}!")
  ELSE()
    IF(NOT External_FIND_QUIETLY)
      MESSAGE(STATUS "Warning: Unable to find ${EXTERNAL_NAME}!")
    ENDIF()
  ENDIF()
ENDIF()

MARK_AS_ADVANCED(EXTERNAL_INCLUDE_PATH EXTERNAL_BINARY_PATH EXTERNAL_LIBRARY_PATH)
