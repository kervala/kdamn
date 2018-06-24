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

FIX_PACKAGE_OPTIONS(MyZLIB ZLIB)
FIND_PACKAGE_HELPER(ZLIB zlib.h RELEASE zlibstatic zdll zlib1 z DEBUG zlibstaticd zdlld zlibd1 zd DIR ${POPPLER_DIR} QUIET)

IF(ZLIB_INCLUDE_DIR AND EXISTS "${ZLIB_INCLUDE_DIR}/zlib.h")
  PARSE_VERSION_OTHER(${ZLIB_INCLUDE_DIR}/zlib.h ZLIB_VERSION ZLIB_VER_MAJOR ZLIB_VER_MINOR ZLIB_VER_REVISION ZLIB_VER_SUBREVISION)

  IF(NOT ZLIB_VER_MAJOR)
    PARSE_VERSION_STRING(${ZLIB_VERSION} ZLIB_VERSION_MAJOR ZLIB_VERSION_MINOR ZLIB_VERSION_PATCH)
  ENDIF()
  
  MESSAGE_VERSION_PACKAGE_HELPER(ZLIB ${ZLIB_VERSION} ${ZLIB_LIBRARIES})
ENDIF()
