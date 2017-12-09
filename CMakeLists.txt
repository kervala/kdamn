#
#  kdAmn is a deviantART Messaging Network client
#  Copyright (C) 2013-2015  Cedric OCHS
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

CMAKE_MINIMUM_REQUIRED(VERSION 2.8.7)

# Allows qtmain to be linked auto
IF(POLICY CMP0020)
  CMAKE_POLICY(SET CMP0020 NEW)
ENDIF()

# Let automoc also process generated files
IF(POLICY CMP0071)
  CMAKE_POLICY(SET CMP0071 NEW)
ENDIF()

# Use AppleClang for OS X compiler
IF(POLICY CMP0025)
  CMAKE_POLICY(SET CMP0025 NEW)
ENDIF()

# Include our generic module with all macros
SET(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules)
INCLUDE(common)

# Define useful variables
SET(VERSION_MAJOR 1)
SET(VERSION_MINOR 2)
SET(VERSION_PATCH "REVISION")
SET(AUTHOR "Kervala")
SET(PRODUCT "kdAmn")
SET(DESCRIPTION "deviantART Messaging Network (dAmn) chat client")
SET(TARGET "kdamn")
SET(YEAR "2013-${CURRENT_YEAR}")

PROJECT(${PRODUCT} C CXX)

# Instruct CMake to run moc automatically when needed.
SET(CMAKE_AUTOMOC ON)

INIT_DEFAULT_OPTIONS()

# Qt doesn't use RTTI or C++ exceptions
SET_OPTION_DEFAULT(WITH_EXCEPTIONS OFF)
SET_OPTION_DEFAULT(WITH_RTTI OFF)
SET_OPTION_DEFAULT(WITH_INSTALL_LIBRARIES OFF)

SETUP_DEFAULT_OPTIONS()

INIT_BUILD_FLAGS()
SETUP_BUILD_FLAGS()

SETUP_PREFIX_PATHS(${TARGET})
SETUP_EXTERNAL()

GEN_CONFIG_H()
GEN_REVISION_H()

USE_QT_MODULES(Gui Network Svg QT4 Script QT5 Multimedia Widgets WinExtras)

IF(UNIX AND NOT APPLE)
  FIND_PACKAGE(X11)

  IF(X11_FOUND)
    INCLUDE_DIRECTORIES(${X11_INCLUDE_DIR})
  ENDIF()
ENDIF()

# To fix compilation of MOC files under Linux
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/src)

FILE(GLOB HEADER src/*.h)
FILE(GLOB SRC src/*.cpp)
FILE(GLOB UI ui/*.ui)
FILE(GLOB RES res/*.qrc res/*.icns res/*.ico)
FILE(GLOB TS translations/*.ts)

SET_TARGET_GUI_EXECUTABLE(${TARGET} ${SRC} ${RES} ${UI} ${HEADER} ${TS} NAME ${PRODUCT} LABEL ${PRODUCT})

IF(APPLE)
  SET(MACOSX_BUNDLE_GUI_IDENTIFIER "net.kervala.${TARGET}")
ENDIF()

# when Qt is static, kdAmn is already linked to X11 and Xmu
IF(NOT QT_STATIC AND X11_FOUND)
  IF(NOT X11_Xmu_LIB)
    FIND_LIBRARY(X11_Xmu_LIB Xmu)
  ENDIF()

  IF(X11_Xmu_LIB)
    TARGET_LINK_LIBRARIES(${TARGET} ${X11_Xmu_LIB})
  ELSE()
    MESSAGE(FATAL_ERROR "Unable to find Xmu!")
  ENDIF()

  TARGET_LINK_LIBRARIES(${TARGET} ${X11_LIBRARIES})
ENDIF()

IF(WITH_PCH)
  ADD_NATIVE_PRECOMPILED_HEADER(${TARGET} ${CMAKE_CURRENT_SOURCE_DIR}/src/common.h ${CMAKE_CURRENT_SOURCE_DIR}/src/common.cpp)
ENDIF()

INSTALL_RESOURCES(${TARGET} misc)
