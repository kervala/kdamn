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

FIND_PACKAGE(MyZLIB REQUIRED)

IF(ZLIB_FOUND)
  SET(_PNG_LIBS_RELEASE)
  SET(_PNG_LIBS_DEBUG)

  SET(_PNG_VERSIONS 18 17 16 15 14 13 12)

  FOREACH(_PNG_VERSION ${_PNG_VERSIONS})
    LIST(APPEND _PNG_LIBS_RELEASE libpng${_PNG_VERSION}_static libpng${_PNG_VERSION} png${_PNG_VERSION})
    LIST(APPEND _PNG_LIBS_DEBUG libpng${_PNG_VERSION}_staticd libpng${_PNG_VERSION}d png${_PNG_VERSION}d)
  ENDFOREACH()

  FIX_PACKAGE_OPTIONS(MyPNG PNG)
  FIND_PACKAGE_HELPER(PNG png.h
    RELEASE ${_PNG_LIBS_RELEASE} libpng_static libpng png
    DEBUG ${_PNG_LIBS_DEBUG} libpng_staticd libpngd pngd QUIET)

  IF(PNG_FOUND)
    # png.h includes zlib.h. Sigh.
    SET(PNG_INCLUDE_DIRS ${PNG_PNG_INCLUDE_DIRS} ${ZLIB_INCLUDE_DIR})
    SET(PNG_LIBRARIES ${PNG_LIBRARIES} ${ZLIB_LIBRARIES})

    IF(CYGWIN)
      IF(BUILD_SHARED_LIBS)
        # No need to define PNG_USE_DLL here, because it's default for Cygwin.
      ELSE()
        SET(PNG_DEFINITIONS -DPNG_STATIC)
      ENDIF()
    ENDIF()
  ENDIF()

  PARSE_VERSION_OTHER(${PNG_INCLUDE_DIR}/png.h PNG_LIBPNG_VER_MAJOR PNG_LIBPNG_VER_MINOR PNG_LIBPNG_VER_RELEASE)
  SET(PNG_VERSION "${PNG_LIBPNG_VER_MAJOR}.${PNG_LIBPNG_VER_MINOR}.${PNG_LIBPNG_VER_RELEASE}")

  MESSAGE_VERSION_PACKAGE_HELPER(PNG ${PNG_VERSION} ${PNG_LIBRARIES})
ENDIF()
