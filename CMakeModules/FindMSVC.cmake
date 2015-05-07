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

# - Find MS Visual C++
#
#  VC_INCLUDE_DIR  - where to find headers
#  VC_INCLUDE_DIRS - where to find headers
#  VC_LIBRARY_DIR  - where to find libraries
#  VC_FOUND        - True if MSVC found.

MACRO(DETECT_VC_VERSION_HELPER _ROOT _VERSION)
  # Software/Wow6432Node/...
  GET_FILENAME_COMPONENT(VC${_VERSION}_DIR "[${_ROOT}\\SOFTWARE\\Microsoft\\VisualStudio\\SxS\\VC7;${_VERSION}]" ABSOLUTE)

  IF(VC${_VERSION}_DIR AND VC${_VERSION}_DIR STREQUAL "/registry")
    SET(VC${_VERSION}_DIR)
    GET_FILENAME_COMPONENT(VC${_VERSION}_DIR "[${_ROOT}\\SOFTWARE\\Microsoft\\VisualStudio\\SxS\\VS7;${_VERSION}]" ABSOLUTE)
    IF(VC${_VERSION}_DIR AND NOT VC${_VERSION}_DIR STREQUAL "/registry")
      SET(VC${_VERSION}_DIR "${VC${_VERSION}_DIR}VC/")
    ENDIF()
  ENDIF()

  IF(VC${_VERSION}_DIR AND NOT VC${_VERSION}_DIR STREQUAL "/registry")
    SET(VC${_VERSION}_FOUND ON)
    DETECT_EXPRESS_VERSION(${_VERSION})
    IF(NOT MSVC_FIND_QUIETLY)
      SET(_VERSION_STR ${_VERSION})
      IF(MSVC_EXPRESS)
        SET(_VERSION_STR "${_VERSION_STR} Express")
      ENDIF()
      MESSAGE(STATUS "Found Visual C++ ${_VERSION_STR} in ${VC${_VERSION}_DIR}")
    ENDIF()
  ELSEIF(VC${_VERSION}_DIR AND NOT VC${_VERSION}_DIR STREQUAL "/registry")
    SET(VC${_VERSION}_FOUND OFF)
    SET(VC${_VERSION}_DIR "")
  ENDIF()
ENDMACRO()

MACRO(DETECT_VC_VERSION _VERSION)
  SET(VC${_VERSION}_FOUND OFF)
  DETECT_VC_VERSION_HELPER("HKEY_CURRENT_USER" ${_VERSION})

  IF(NOT VC${_VERSION}_FOUND)
    DETECT_VC_VERSION_HELPER("HKEY_LOCAL_MACHINE" ${_VERSION})
  ENDIF()

  IF(VC${_VERSION}_FOUND)
    SET(VC_FOUND ON)
    SET(VC_DIR "${VC${_VERSION}_DIR}")
  ENDIF()
ENDMACRO()

MACRO(DETECT_EXPRESS_VERSION _VERSION)
  GET_FILENAME_COMPONENT(MSVC_EXPRESS "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\VCExpress\\${_VERSION}\\Setup\\VC;ProductDir]" ABSOLUTE)

  IF(MSVC_EXPRESS AND NOT MSVC_EXPRESS STREQUAL "/registry")
    SET(MSVC_EXPRESS ON)
  ENDIF()
ENDMACRO()

IF(MSVC12)
  DETECT_VC_VERSION("12.0")
  SET(MSVC_TOOLSET "120")

  IF(NOT MSVC12_REDIST_DIR)
    # If you have VC++ 2013 Express, put x64/Microsoft.VC120.CRT/*.dll in ${EXTERNAL_PATH}/redist
    SET(MSVC12_REDIST_DIR "${EXTERNAL_PATH}/redist")
  ENDIF()
ELSEIF(MSVC11)
  DETECT_VC_VERSION("11.0")
  SET(MSVC_TOOLSET "110")

  IF(NOT MSVC11_REDIST_DIR)
    # If you have VC++ 2012 Express, put x64/Microsoft.VC110.CRT/*.dll in ${EXTERNAL_PATH}/redist
    SET(MSVC11_REDIST_DIR "${EXTERNAL_PATH}/redist")
  ENDIF()
ELSEIF(MSVC10)
  DETECT_VC_VERSION("10.0")
  SET(MSVC_TOOLSET "100")

  IF(NOT MSVC10_REDIST_DIR)
    # If you have VC++ 2010 Express, put x64/Microsoft.VC100.CRT/*.dll in ${EXTERNAL_PATH}/redist
    SET(MSVC10_REDIST_DIR "${EXTERNAL_PATH}/redist")
  ENDIF()
ELSEIF(MSVC90)
  DETECT_VC_VERSION("9.0")
  SET(MSVC_TOOLSET "90")
ELSEIF(MSVC80)
  DETECT_VC_VERSION("8.0")
  SET(MSVC_TOOLSET "80")
ENDIF()

# If you plan to use VC++ compilers with WINE, set VC_DIR environment variable
IF(NOT VC_DIR)
  SET(VC_DIR $ENV{VC_DIR})
ENDIF()

IF(NOT VC_DIR)
  IF(CMAKE_CXX_COMPILER)
    SET(_COMPILER ${CMAKE_CXX_COMPILER})
  ELSE()
    SET(_COMPILER ${CMAKE_C_COMPILER})
  ENDIF()
  STRING(REGEX REPLACE "/bin/.+" "" VC_DIR ${_COMPILER})
ENDIF()

SET(VC_INCLUDE_DIR "${VC_DIR}/include")
SET(VC_INCLUDE_DIRS ${VC_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${VC_INCLUDE_DIR})
