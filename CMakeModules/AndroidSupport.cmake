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

MACRO(INIT_BUILD_FLAGS_ANDROID)
  IF(ANDROID)
    # Generic compiler flags
    SET(PLATFORM_CFLAGS "${PLATFORM_CFLAGS} ${ANDROID_COMPILER_FLAGS}")
    SET(PLATFORM_CXXFLAGS "${PLATFORM_CXXFLAGS} ${ANDROID_COMPILER_FLAGS} ${ANDROID_COMPILER_FLAGS_CXX}")
    SET(PLATFORM_EXE_LINKFLAGS "${PLATFORM_EXE_LINKFLAGS} ${ANDROID_LINKER_FLAGS} ${ANDROID_LINKER_FLAGS_EXE}")
    SET(PLATFORM_MODULE_LINKFLAGS "${PLATFORM_MODULE_LINKFLAGS} ${ANDROID_LINKER_FLAGS}")
    SET(PLATFORM_SHARED_LINKFLAGS "${PLATFORM_SHARED_LINKFLAGS} ${ANDROID_LINKER_FLAGS}")

    # Debug
    SET(DEBUG_CFLAGS "${DEBUG_CFLAGS} ${ANDROID_COMPILER_FLAGS_DEBUG}")

    # Release
    SET(RELEASE_CFLAGS "${RELEASE_CFLAGS} ${ANDROID_COMPILER_FLAGS_RELEASE}")

    # Define architecture
    SET(CMAKE_LIBRARY_ARCHITECTURE ${ANDROID_ABI})
    SET(LIBRARY_ARCHITECTURE ${ANDROID_ABI})

    # Append search path for libraries
    LIST(APPEND CMAKE_FIND_ROOT_PATH ${CMAKE_PREFIX_PATH} ${CMAKE_INSTALL_PREFIX} $ENV{EXTERNAL_ANDROID_PATH})

    IF(TARGET_ARM)
      ADD_PLATFORM_FLAGS("-D__ARM_ARCH_5__ -D__ARM_ARCH_5T__ -D__ARM_ARCH_5E__ -D__ARM_ARCH_5TE__")

      IF(TARGET_ARMV7)
        ADD_PLATFORM_FLAGS("-D__ARM_V7__")
      ENDIF()

      IF(ANDROID_ARM_MODE STREQUAL "thumb")
        SET(TARGET_THUMB ON)
      ELSE()
        SET(TARGET_THUMB OFF)
      ENDIF()
    ENDIF()
  ENDIF()
ENDMACRO()
