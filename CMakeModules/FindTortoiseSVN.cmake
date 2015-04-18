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

FIND_PROGRAM(TORTOISESVN_EXECUTABLE SubWCRev
  PATHS
    "C:/Program Files/TortoiseSVN/bin"
    "C:/Program Files (x86)/TortoiseSVN/bin"
  DOC "TortoiseSVN command line client")
MARK_AS_ADVANCED(TORTOISESVN_EXECUTABLE)

IF(TORTOISESVN_EXECUTABLE)
  SET(TORTOISESVN_FOUND ON)

  EXECUTE_PROCESS(COMMAND ${TORTOISESVN_EXECUTABLE}
    OUTPUT_VARIABLE TORTOISESVN_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE)

  STRING(REGEX REPLACE "^(.*\n)?SubWCRev ([.0-9]+), Build ([0-9]+).*" "\\2.\\3" TORTOISESVN_VERSION "${TORTOISESVN_VERSION}")

  MESSAGE(STATUS "Found TortoiseSVN ${TORTOISESVN_VERSION}")

  MACRO(TORTOISESVN_GET_REVISION dir rev)
    EXECUTE_PROCESS(COMMAND ${TORTOISESVN_EXECUTABLE} ${dir}
      OUTPUT_VARIABLE _OUTPUT
      ERROR_VARIABLE _ERROR
      RESULT_VARIABLE _RESULT
      OUTPUT_STRIP_TRAILING_WHITESPACE)

    IF(NOT ${_RESULT} EQUAL 0)
      MESSAGE(SEND_ERROR "Command \"${TORTOISESVN_EXECUTABLE} ${dir}\" failed with output:\n${_ERROR}")
    ELSE()
      STRING(REGEX REPLACE "^(.*\n)?Last committed at revision ([0-9]+).*" "\\2" _REVISION "${_OUTPUT}")
      
      IF(_REVISION)
        SET(${rev} ${_REVISION})
      ENDIF()
    ENDIF()
  ENDMACRO()
ENDIF()
