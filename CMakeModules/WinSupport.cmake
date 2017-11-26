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

# Read CONFIGURATION environment variable
SET(ENV_CONFIGURATION $ENV{CONFIGURATION})

# Force configuration set by Windows SDK
IF(NOT CMAKE_BUILD_TYPE AND ENV_CONFIGURATION)
  SET(CMAKE_BUILD_TYPE ${ENV_CONFIGURATION} CACHE STRING "" FORCE)
ENDIF()

MACRO(SIGN_FILE_WINDOWS TARGET)
  IF(WITH_SIGN_FILE AND WINSDK_SIGNTOOL AND CMAKE_BUILD_TYPE STREQUAL "Release")
    GET_TARGET_PROPERTY(filename ${TARGET} LOCATION)
#    ADD_CUSTOM_COMMAND(
#      TARGET ${target}
#      POST_BUILD
#      COMMAND ${WINSDK_SIGNTOOL} sign ${filename}
#      VERBATIM)
  ENDIF()
ENDMACRO()

################################################################################
# MACRO_ADD_INTERFACES(idl_files...)
#
# Syntax: MACRO_ADD_INTERFACES(<output list> <idl1> [<idl2> [...]])
# Notes: <idl1> should be absolute paths so the MIDL compiler can find them.
# For every idl file xyz.idl, two files xyz_h.h and xyz.c are generated, which
# are added to the <output list>

# Copyright (c) 2007, Guilherme Balena Versiani, <[EMAIL PROTECTED]>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.
MACRO (MACRO_ADD_INTERFACES _output_list)
  IF(NOT WINSDK_MIDL)
    MESSAGE(FATAL_ERROR "midl not found, please check your Windows SDK installation")
    RETURN()
  ENDIF()

  FOREACH(_in_FILE ${ARGN})
    GET_FILENAME_COMPONENT(_out_FILE ${_in_FILE} NAME_WE)
    GET_FILENAME_COMPONENT(_in_PATH ${_in_FILE} PATH)

    SET(_out_header_name ${_out_FILE}.h)
    SET(_out_header ${CMAKE_CURRENT_BINARY_DIR}/${_out_header_name})
    SET(_out_iid_name ${_out_FILE}_i.c)
    SET(_out_iid ${CMAKE_CURRENT_BINARY_DIR}/${_out_iid_name})
    #message("_out_header_name=${_out_header_name}, _out_header=${_out_header}, _out_iid=${_out_iid}")
    ADD_CUSTOM_COMMAND(
      OUTPUT ${_out_header} ${_out_iid}
      DEPENDS ${_in_FILE}
      COMMAND ${WINSDK_MIDL} /nologo /char signed /env win32 /Oicf /header ${_out_header_name} /iid ${_out_iid_name} /I ${WINSDK_INCLUDE_DIR} ${_in_FILE}
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )

    SET_PROPERTY(SOURCE ${_out_header} APPEND PROPERTY OBJECT_DEPENDS ${_in_FILE})

    SET_SOURCE_FILES_PROPERTIES(${_out_header} ${_out_iid} PROPERTIES GENERATED TRUE)
    SET_SOURCE_FILES_PROPERTIES(${_in_FILE} PROPERTIES HEADER_FILE_ONLY TRUE)

    SET(${_output_list} ${${_output_list}} ${_out_header})
  ENDFOREACH()
ENDMACRO (MACRO_ADD_INTERFACES)

################################################################################
# macros to manage PDB files
################################################################################

MACRO(GET_INTERMEDIATE_PDB_FILENAME name output)
  # determine output directory based on target type
  GET_TARGET_PROPERTY(_targetType ${name} TYPE)

  IF(${_targetType} STREQUAL STATIC_LIBRARY)
    # intermediate and final PDB are identical
    GET_FINAL_PDB_FILENAME(${name} output)
  ELSE()
    # determine target prefix
    GET_TARGET_PROPERTY(_targetPrefix ${name} PREFIX)
    IF(${_targetPrefix} MATCHES NOTFOUND)
      SET(_targetPrefix "")
    ENDIF()

    SET(${output} "${_targetPrefix}vc${MSVC_TOOLSET}")
  ENDIF()
ENDMACRO()

MACRO(GET_INTERMEDIATE_PDB_DIRECTORY name output)
  # determine output directory based on target type
  GET_TARGET_PROPERTY(_targetType ${name} TYPE)

  IF(${_targetType} STREQUAL STATIC_LIBRARY)
    # intermediate and final PDB are identical
    GET_FINAL_PDB_DIRECTORY(${name} output)
  ELSE()
    # set it to a temporary directory because the true PDB file is set using /PDB instead of /Fd
    # use a simple path to put intermediary PDB file
    IF(NMAKE)
      SET(${output} "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${name}.dir")
    ELSE()
      SET(${output} "${CMAKE_CURRENT_BINARY_DIR}/${name}.dir/Debug")
    ENDIF()
  ENDIF()
ENDMACRO()

MACRO(GET_INTERMEDIATE_PDB_FULLPATH name output)
  GET_INTERMEDIATE_PDB_FILENAME(${name} _PDB_FILENAME)
  GET_INTERMEDIATE_PDB_DIRECTORY(${name} _PDB_DIRECTORY)

  SET(${output} "${_PDB_DIRECTORY}/${_PDB_FILENAME}.pdb")
ENDMACRO()

MACRO(GET_FINAL_PDB_FILENAME name output)
  # determine target prefix
  GET_TARGET_PROPERTY(_targetPrefix ${name} PREFIX)
  IF(${_targetPrefix} MATCHES NOTFOUND)
    SET(_targetPrefix "")
  ENDIF()

  # determine target postfix
  GET_TARGET_PROPERTY(_targetPostfix ${name} DEBUG_POSTFIX)

  IF(${_targetPostfix} MATCHES NOTFOUND)
    SET(_targetPostfix "${_DEBUG_POSTFIX}")
  ENDIF()

  SET(${output} "${_targetPrefix}${name}${_targetPostfix}")
ENDMACRO()

MACRO(GET_FINAL_PDB_DIRECTORY name output)
  # determine output directory based on target type
  GET_TARGET_PROPERTY(_targetType ${name} TYPE)

  # use correct output path
  IF(${_targetType} STREQUAL STATIC_LIBRARY)
    SET(_PDB_DIRECTORY ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})
  ELSE()
    SET(_PDB_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
  ENDIF()

  # fix location depending on using NMake or VC++
  IF(NMAKE)
    SET(_PDB_DIRECTORY ${_PDB_DIRECTORY})
  ELSE()
    SET(_PDB_DIRECTORY ${_PDB_DIRECTORY}/Debug)
  ENDIF()

  SET(${output} ${_PDB_DIRECTORY})
ENDMACRO()

MACRO(GET_FINAL_PDB_FULLPATH name output)
  GET_FINAL_FILENAME_FULLPATH_WITH_EXTENSION(${name} ${output} "pdb")
ENDMACRO()

MACRO(GET_FINAL_FILENAME_FULLPATH name output)
  GET_FINAL_PDB_DIRECTORY(${name} _PDB_DIRECTORY)
  GET_FINAL_PDB_FILENAME(${name} _PDB_FILENAME)

  SET(${output} "${_PDB_DIRECTORY}/${_PDB_FILENAME}")
ENDMACRO()

MACRO(GET_FINAL_FILENAME_FULLPATH_WITH_EXTENSION name output ext)
  GET_FINAL_FILENAME_FULLPATH(${name} ${output})

  SET(${output} "${${output}}.${ext}")
ENDMACRO()

MACRO(SET_TARGET_FLAGS_MSVC name)
  IF(MSVC)
    GET_TARGET_PROPERTY(type ${name} TYPE)

    # define linker version
    IF(NOT "${type}" STREQUAL STATIC_LIBRARY)
      # check if using a GUI
      GET_TARGET_PROPERTY(_VALUE ${name} WIN32_EXECUTABLE)

      IF(TARGET_X64)
        # Target Windows XP 64 bits
        SET(_SUBSYSTEM_VERSION "5.2")
      ELSE()
        # Target Windows XP
        SET(_SUBSYSTEM_VERSION "5.1")
      ENDIF()

      IF(_VALUE)
        # GUI
        SET(_SUBSYSTEM "WINDOWS")
      ELSE()
        # Console
        SET(_SUBSYSTEM "CONSOLE")
      ENDIF()

      GET_TARGET_PROPERTY(_LINK_FLAGS ${name} LINK_FLAGS)

      IF(NOT _LINK_FLAGS)
        SET(_LINK_FLAGS "")
      ENDIF()

      SET_TARGET_PROPERTIES(${name} PROPERTIES LINK_FLAGS "/VERSION:${VERSION_MAJOR}.${VERSION_MINOR} ${_LINK_FLAGS}")
    ENDIF()

    IF("${type}" STREQUAL "STATIC_LIBRARY")
      # final and intermediate PDB are identical
      GET_FINAL_PDB_FILENAME(${name} _PDB_FILENAME)
      GET_FINAL_PDB_DIRECTORY(${name} _PDB_DIRECTORY)

      # Remove lib prefix if CMake added it itself
      IF(WITH_PREFIX_LIB)
        STRING(REGEX REPLACE "^lib" "" _PDB_FILENAME ${_PDB_FILENAME})
      ENDIF()

      # define all properties supported by CMake for PDB (if not supported, it won't change anything)
      SET_TARGET_PROPERTIES(${name} PROPERTIES COMPILE_PDB_OUTPUT_DIRECTORY_DEBUG "${_PDB_DIRECTORY}" COMPILE_PDB_NAME_DEBUG "${_PDB_FILENAME}")
      SET_TARGET_PROPERTIES(${name} PROPERTIES PDB_OUTPUT_DIRECTORY_DEBUG "${_PDB_DIRECTORY}" PDB_NAME_DEBUG "${_PDB_FILENAME}")
    ELSEIF("${type}" STREQUAL "EXECUTABLE")
      SET_TARGET_PROPERTIES(${name} PROPERTIES SOVERSION ${VERSION_MAJOR})
    ELSEIF("${type}" STREQUAL "SHARED_LIBRARY")
      # final PDB
      GET_FINAL_PDB_FILENAME(${name} _PDB_FILENAME)
      GET_FINAL_PDB_DIRECTORY(${name} _PDB_DIRECTORY)

      # Remove lib prefix if CMake added it itself
      IF(WITH_PREFIX_LIB)
        STRING(REGEX REPLACE "^lib" "" _PDB_FILENAME ${_PDB_FILENAME})
      ENDIF()

      # intermediate PDB
      GET_INTERMEDIATE_PDB_FILENAME(${name} _COMPILE_PDB_FILENAME)
      GET_INTERMEDIATE_PDB_DIRECTORY(${name} _COMPILE_PDB_DIRECTORY)

      # Remove lib prefix if CMake added it itself
      IF(WITH_PREFIX_LIB)
        STRING(REGEX REPLACE "^lib" "" _COMPILE_PDB_FILENAME ${_COMPILE_PDB_FILENAME})
      ENDIF()

      # define all properties supported by CMake for PDB (if not supported, it won't change anything)
      SET_TARGET_PROPERTIES(${name} PROPERTIES COMPILE_PDB_OUTPUT_DIRECTORY_DEBUG "${_COMPILE_PDB_DIRECTORY}" COMPILE_PDB_NAME_DEBUG "${_COMPILE_PDB_FILENAME}")
      SET_TARGET_PROPERTIES(${name} PROPERTIES PDB_OUTPUT_DIRECTORY_DEBUG "${_PDB_DIRECTORY}" PDB_NAME_DEBUG "${_PDB_FILENAME}")
    ENDIF()

    IF(MANUALLY_MANAGE_PDB_FLAG)
      GET_INTERMEDIATE_PDB_FULLPATH(${name} _PDB_FULLPATH)

      # define /Fd flag manually for PDB
      SET_PROPERTY(TARGET ${name} APPEND_STRING PROPERTY COMPILE_FLAGS " /Fd${_PDB_FULLPATH}")
    ENDIF()
  ENDIF()
ENDMACRO()

MACRO(INIT_BUILD_FLAGS_WINDOWS)
  IF(WIN32)
    IF(MSVC)
      # From 2.8.12 (included) to 3.1.0 (excluded), the /Fd parameter to specify
      # compilation PDB was managed entirely by CMake and there was no way to access
      # or change it, so we need to remove it from CFLAGS and manage it ourself later
      IF(CMAKE_VERSION VERSION_GREATER "2.8.11.9" AND CMAKE_VERSION VERSION_LESS "3.1.0")
        SET(MANUALLY_MANAGE_PDB_FLAG ON)
      ELSE()
        SET(MANUALLY_MANAGE_PDB_FLAG OFF)
      ENDIF()

      # VC++ versions
      #
      # year public private dll  registry
      #
      # 2017 14.11  1911    140  15.0
      # 2017 14.1   1910    140  15.0
      # 2015 14     1900    140  14.0
      # 2013 12     1800    120
      # 2012 11     1700    110
      # 2010 10     1600    100
      # 2008  9     1500    90
      # 2005  8     1400    80
      # 2003  7.1   1310    71
      # 2002  7     1300    70
      # 1998  6     1200    60
      # 1997  5     1100    50
      # 1995  4     1000    40

      # Ignore default include paths
      ADD_PLATFORM_FLAGS("/X")

      # global optimizations
      # SET(RELEASE_CFLAGS "/GL ${RELEASE_CFLAGS}")
      # SET(RELEASE_LINKFLAGS "/LTCG:incremental ${RELEASE_LINKFLAGS}")

      IF(MSVC14)
        ADD_PLATFORM_FLAGS("/Gy-")
        # /Ox is working with VC++ 2017, but custom optimizations don't exist
        SET(RELEASE_CFLAGS "/Ox /GF /GS- ${RELEASE_CFLAGS}")
        # without inlining it's unusable, use custom optimizations again
        SET(DEBUG_CFLAGS "/Od /Ob1 /GF- ${DEBUG_CFLAGS}")

        IF(WITH_INSTALL_RUNTIMES)
          SET(CMAKE_INSTALL_UCRT_LIBRARIES ON)
        ENDIF()

        # Special cases for VC++ 2017
        IF(MSVC_VERSION EQUAL "1911")
          SET(MSVC1411 ON)
        ELSEIF(MSVC_VERSION EQUAL "1910")
          SET(MSVC1410 ON)
        ENDIF()
      ELSEIF(MSVC12)
        ADD_PLATFORM_FLAGS("/Gy-")
        # /Ox is working with VC++ 2013, but custom optimizations don't exist
        SET(RELEASE_CFLAGS "/Ox /GF /GS- ${RELEASE_CFLAGS}")
        # without inlining it's unusable, use custom optimizations again
        SET(DEBUG_CFLAGS "/Od /Ob1 /GF- ${DEBUG_CFLAGS}")
      ELSEIF(MSVC11)
        ADD_PLATFORM_FLAGS("/Gy-")
        # /Ox is working with VC++ 2012, but custom optimizations don't exist
        SET(RELEASE_CFLAGS "/Ox /GF /GS- ${RELEASE_CFLAGS}")
        # without inlining it's unusable, use custom optimizations again
        SET(DEBUG_CFLAGS "/Od /Ob1 /GF- ${DEBUG_CFLAGS}")
      ELSEIF(MSVC10)
        ADD_PLATFORM_FLAGS("/Gy-")
        # /Ox is working with VC++ 2010, but custom optimizations don't exist
        SET(RELEASE_CFLAGS "/Ox /GF /GS- ${RELEASE_CFLAGS}")
        # without inlining it's unusable, use custom optimizations again
        SET(DEBUG_CFLAGS "/Od /Ob1 /GF- ${DEBUG_CFLAGS}")
      ELSEIF(MSVC90)
        ADD_PLATFORM_FLAGS("/Gy-")
        # don't use a /O[012x] flag if you want custom optimizations
        SET(RELEASE_CFLAGS "/Ob2 /Oi /Ot /Oy /GT /GF /GS- ${RELEASE_CFLAGS}")
        # without inlining it's unusable, use custom optimizations again
        SET(DEBUG_CFLAGS "/Ob1 /GF- ${DEBUG_CFLAGS}")
      ELSEIF(MSVC80)
        ADD_PLATFORM_FLAGS("/Gy- /Wp64")
        # don't use a /O[012x] flag if you want custom optimizations
        SET(RELEASE_CFLAGS "/Ox /GF /GS- ${RELEASE_CFLAGS}")
        # without inlining it's unusable, use custom optimizations again
        SET(DEBUG_CFLAGS "/Od /Ob1 ${DEBUG_CFLAGS}")
      ELSE()
        MESSAGE(FATAL_ERROR "Can't determine compiler version ${MSVC_VERSION}")
      ENDIF()

      ADD_PLATFORM_FLAGS("/D_CRT_SECURE_NO_DEPRECATE /D_CRT_SECURE_NO_WARNINGS /D_CRT_NONSTDC_NO_WARNINGS /D_SCL_SECURE_NO_WARNINGS /D_WIN32 /DWIN32 /D_WINDOWS /wd4250")

      IF(WITH_PCH_MAX_SIZE)
        ADD_PLATFORM_FLAGS("/Zm1000")
      ENDIF()

      IF(TARGET_X64)
        # Target Vista for x64
        ADD_PLATFORM_FLAGS("/D_WIN32_WINNT=0x0600 /DWINVER=0x0600")

        # Fix a bug with Intellisense
        ADD_PLATFORM_FLAGS("/D_WIN64")
        # Fix a compilation error for some big C++ files
        ADD_PLATFORM_FLAGS("/bigobj")
      ELSE()
        # Target XP for x86
        ADD_PLATFORM_FLAGS("/D_WIN32_WINNT=0x0501 /DWINVER=0x0501")

        # Allows 32 bits applications to use 3 GB of RAM
        ADD_PLATFORM_LINKFLAGS("/LARGEADDRESSAWARE")
      ENDIF()

      # Exceptions are only set for C++
      IF(WITH_EXCEPTIONS)
        SET(PLATFORM_CXXFLAGS "${PLATFORM_CXXFLAGS} /EHa")
      ELSE()
        SET(PLATFORM_CXXFLAGS "${PLATFORM_CXXFLAGS} -DBOOST_NO_EXCEPTIONS -D_HAS_EXCEPTIONS=0 /wd4275 /wd4577")
      ENDIF()

      # RTTI is only set for C++
      IF(WITH_RTTI)
#       SET(PLATFORM_CXXFLAGS "${PLATFORM_CXXFLAGS} /GR")
      ELSE()
        SET(PLATFORM_CXXFLAGS "${PLATFORM_CXXFLAGS} /GR-")
      ENDIF()

      IF(WITH_SYMBOLS)
        SET(RELEASE_CFLAGS "/Zi ${RELEASE_CFLAGS}")
        SET(RELEASE_LINKFLAGS "/DEBUG ${RELEASE_LINKFLAGS}")
      ELSE()
        SET(RELEASE_LINKFLAGS "/RELEASE ${RELEASE_LINKFLAGS}")
      ENDIF()

      IF(WITH_STATIC_RUNTIMES)
        SET(RUNTIME_FLAG "/MT")
      ELSE()
        SET(RUNTIME_FLAG "/MD")
      ENDIF()

      IF(WITH_WARNINGS)
        SET(DEBUG_CFLAGS "/W4 ${DEBUG_CFLAGS}")
      ELSE()
        SET(DEBUG_CFLAGS "/W3 ${DEBUG_CFLAGS}")
      ENDIF()

      SET(DEBUG_CFLAGS "/Zi ${RUNTIME_FLAG}d /RTC1 /D_DEBUG /DDEBUG ${DEBUG_CFLAGS}")
      SET(RELEASE_CFLAGS "${RUNTIME_FLAG} /DNDEBUG ${RELEASE_CFLAGS}")
      SET(DEBUG_LINKFLAGS "/DEBUG /OPT:NOREF /OPT:NOICF /NODEFAULTLIB:msvcrt /PDBCOMPRESS ${MSVC_INCREMENTAL_YES_FLAG} ${DEBUG_LINKFLAGS}")
      SET(RELEASE_LINKFLAGS "/OPT:REF /OPT:ICF /INCREMENTAL:NO ${RELEASE_LINKFLAGS}")

      IF(MANUALLY_MANAGE_PDB_FLAG)
        SET(CMAKE_C_COMPILE_OBJECT "<CMAKE_C_COMPILER> /nologo /TC <FLAGS> <DEFINES> /Fo<OBJECT> -c <SOURCE>")
        SET(CMAKE_CXX_COMPILE_OBJECT "<CMAKE_CXX_COMPILER> /nologo /TP <FLAGS> <DEFINES> /Fo<OBJECT> -c <SOURCE>")
      ENDIF()

      IF(TARGET_X64)
        # Target Vista 64 bits
        SET(SUBSYSTEM_VERSION "6.0")
      ELSE()
        # Target Windows XP
        SET(SUBSYSTEM_VERSION "5.1")
      ENDIF()

      SET(CMAKE_CREATE_WIN32_EXE "/SUBSYSTEM:WINDOWS,${SUBSYSTEM_VERSION}")
      SET(CMAKE_CREATE_CONSOLE_EXE "/SUBSYSTEM:CONSOLE,${SUBSYSTEM_VERSION}")
    ELSE()
      ADD_PLATFORM_FLAGS("-DWIN32 -D_WIN32")

      IF(CLANG)
        ADD_PLATFORM_FLAGS("-nobuiltininc")
      ENDIF()
    ENDIF()
  ENDIF()
ENDMACRO()

MACRO(INSTALL_PLUGIN_PDB name)
  # copy also PDB files in installation directory for Visual C++
  IF(MSVC AND WITH_INSTALL_LIBRARIES)
    # Destination PDB filename
    GET_FINAL_PDB_FULLPATH(${name} _OUTPUT_FULLPATH)

    IF(WITH_STATIC_PLUGINS)
      # copy PDB file together with lib
      INSTALL(FILES ${_OUTPUT_FULLPATH} DESTINATION ${LIB_PREFIX} CONFIGURATIONS Debug)
    ELSE()
      # copy PDB file together with DLL
      INSTALL(FILES ${_OUTPUT_FULLPATH} DESTINATION ${PLUGIN_PREFIX} CONFIGURATIONS Debug)
    ENDIF()
  ENDIF()
ENDMACRO()

MACRO(INSTALL_LIBRARY_PDB name)
  # copy also PDB files in installation directory for Visual C++
  IF(MSVC)
    IF(IS_STATIC)
      IF(STATIC_LIB)
        SET(_name "${name}_static")
      ELSE()
        SET(_name "${name}")
      ENDIF()

      # Destination PDB filename
      GET_FINAL_PDB_FULLPATH(${_name} _OUTPUT_FULLPATH)

      # copy PDB file together with LIB
      INSTALL(FILES ${_OUTPUT_FULLPATH} DESTINATION ${LIB_PREFIX} CONFIGURATIONS Debug)
    ENDIF()
    IF(IS_SHARED)
      # Destination PDB filename
      GET_FINAL_PDB_FULLPATH(${name} _OUTPUT_FULLPATH)

      # copy PDB file together with DLL
      INSTALL(FILES ${_OUTPUT_FULLPATH} DESTINATION ${BIN_PREFIX} CONFIGURATIONS Debug)
    ENDIF()
  ENDIF()
ENDMACRO()

MACRO(CREATE_NSIS_PACKAGE _TARGET)
  IF(TARGET_X64)
    SET(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES64")
  ELSE()
    SET(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES")
  ENDIF()

  # NSIS Specific Packing Setup
  SET(CPACK_PACKAGE_INSTALL_REGISTRY_KEY ${PRODUCT})
  SET(CPACK_NSIS_EXECUTABLES_DIRECTORY ".")
  #SET(CPACK_NSIS_CREATE_ICONS "CreateShortCut '\$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\${PRODUCT}.lnk' '\$INSTDIR\\\\${TARGET}.exe'")
  #SET(CPACK_NSIS_MODIFY_PATH ON)
  SET(CPACK_NSIS_MUI_ICON ${TARGET_ICON})
  SET(CPACK_NSIS_MUI_UNIICON ${TARGET_ICON})
  #SET(CPACK_NSIS_MUI_WELCOMEFINISHPAGE_BITMAP "${CMAKE_SOURCE_DIR}/res\\\\wizard.bmp") # 164x314
  #SET(CPACK_NSIS_MUI_UNWELCOMEFINISHPAGE_BITMAP "${CMAKE_SOURCE_DIR}/res\\\\wizard.bmp") # 164x314
  #SET(CPACK_NSIS_MUI_FINISHPAGE_RUN "${TARGET}.exe") # Run executable as Admin
  SET(CPACK_NSIS_INSTALLED_ICON_NAME "${_TARGET}.exe")
  SET(CPACK_PACKAGE_ICON "${CMAKE_SOURCE_DIR}/res\\\\header.bmp") # 150x57
  SET(CPACK_NSIS_DISPLAY_NAME ${PRODUCT})
  SET(CPACK_NSIS_HELP_LINK "http://dev.kervala.net/projects/${_TARGET}")
  SET(CPACK_NSIS_URL_INFO_ABOUT "http://dev.kervala.net/projects/${_TARGET}")
  SET(CPACK_NSIS_CONTACT "kervala@gmail.com")
  SET(CPACK_NSIS_COMPRESSOR "/SOLID lzma")

  SET(CPACK_GENERATOR "NSIS")
ENDMACRO()
