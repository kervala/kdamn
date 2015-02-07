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
ENDMACRO(SIGN_FILE_WINDOWS)

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
  ENDFOREACH(_in_FILE ${ARGN})
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
    SET(${output} "vc${MSVC_TOOLSET}")
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
  GET_FINAL_PDB_DIRECTORY(${name} _PDB_DIRECTORY)
  GET_FINAL_PDB_FILENAME(${name} _PDB_FILENAME)

  SET(${output} "${_PDB_DIRECTORY}/${_PDB_FILENAME}.pdb")
ENDMACRO()

MACRO(SET_TARGET_FLAGS_MSVC name)
  IF(MSVC)
    GET_TARGET_PROPERTY(type ${name} TYPE)

    # define linker version
    IF(NOT "${type}" STREQUAL STATIC_LIBRARY)
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

      # define all properties supported by CMake for PDB (if not supported, it won't change anything)
      SET_TARGET_PROPERTIES(${name} PROPERTIES COMPILE_PDB_OUTPUT_DIRECTORY_DEBUG "${_PDB_DIRECTORY}" COMPILE_PDB_NAME_DEBUG "${_PDB_FILENAME}")
      SET_TARGET_PROPERTIES(${name} PROPERTIES PDB_OUTPUT_DIRECTORY_DEBUG "${_PDB_DIRECTORY}" PDB_NAME_DEBUG "${_PDB_FILENAME}")
    ELSEIF("${type}" STREQUAL "EXECUTABLE")
      SET_TARGET_PROPERTIES(${name} PROPERTIES COMPILE_FLAGS "/GA")
      SET_TARGET_PROPERTIES(${name} PROPERTIES VERSION ${VERSION} SOVERSION ${VERSION_MAJOR})
    ELSEIF("${type}" STREQUAL "SHARED_LIBRARY")
      # final PDB
      GET_FINAL_PDB_FILENAME(${name} _PDB_FILENAME)
      GET_FINAL_PDB_DIRECTORY(${name} _PDB_DIRECTORY)

      # intermediate PDB
      GET_INTERMEDIATE_PDB_FILENAME(${name} _COMPILE_PDB_FILENAME)
      GET_INTERMEDIATE_PDB_DIRECTORY(${name} _COMPILE_PDB_DIRECTORY)

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
