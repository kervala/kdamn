# Read CONFIGURATION environment variable
SET(ENV_CONFIGURATION $ENV{CONFIGURATION})

# Force configuration set by Windows SDK
IF(NOT CMAKE_BUILD_TYPE AND ENV_CONFIGURATION)
  SET(CMAKE_BUILD_TYPE ${ENV_CONFIGURATION} CACHE STRING "" FORCE)
ENDIF(NOT CMAKE_BUILD_TYPE AND ENV_CONFIGURATION)

MACRO(SIGN_FILE_WINDOWS TARGET)
  IF(WITH_SIGN_FILE AND WINSDK_SIGNTOOL AND CMAKE_BUILD_TYPE STREQUAL "Release")
    GET_TARGET_PROPERTY(filename ${TARGET} LOCATION)
#    ADD_CUSTOM_COMMAND(
#      TARGET ${target}
#      POST_BUILD
#      COMMAND ${WINSDK_SIGNTOOL} sign ${filename}
#      VERBATIM)
  ENDIF(WITH_SIGN_FILE AND WINSDK_SIGNTOOL AND CMAKE_BUILD_TYPE STREQUAL "Release")
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
  ENDIF(NOT WINSDK_MIDL)

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
