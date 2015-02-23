# Globals variables
# USE_QT
# USE_QT4 or USE_QT5
# QT_BINARY_DIR
# QT_TRANSLATIONS_DIR
# QT_PLUGINS_DIR
# QT_TSS => QT_QMS
# QT_UIS => QT_UIS_HEADERS
# QT_QRCS => QT_QRCS_CPPS
# headers => QT_MOCS_CPPS
# QT_SOURCES
# QT_LANGS
# QT_SHARED_MODULES
# QT4_MODULES
# QT5_MODULES
# QT_MODULES_WANTED
# QT_MODULES_USED

# Check .desktop file under Gnome
SET(DESKTOP_FILE $ENV{GIO_LAUNCHED_DESKTOP_FILE})

# Check build directory
IF(NOT DESKTOP_FILE)
  SET(DESKTOP_FILE ${CMAKE_BINARY_DIR})
ENDIF()

# Force Debug configuration if launched from Qt Creator
IF(NOT CMAKE_BUILD_TYPE AND DESKTOP_FILE MATCHES "qtcreator")
  SET(CMAKE_BUILD_TYPE "Debug" CACHE STRING "" FORCE)
ENDIF()

MACRO(INIT_QT)
  # Init all variables needed by Qt
  SET(QT_SOURCES)
  SET(QT_LANGS)
  SET(QT_MOCS)
  SET(QT_MOCS_CPPS)
  SET(QT_UIS)
  SET(QT_UIS_HEADERS)
  SET(QT_QRCS)
  SET(QT_QRCS_CPPS)
  SET(QT_TSS)
  SET(QT_QMS)
  
  # Regex filter for Qt files
  SET(QT_FILES_FILTER "\\.(ts|qrc|ui)$")

  INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR})
ENDMACRO()

MACRO(ADD_QT5_DEPENDENCIES)
  LIST(APPEND QT_MODULES_WANTED Core LinguistTools Concurrent)

  FOREACH(_MODULE ${QT_MODULES_WANTED})
    IF(_MODULE STREQUAL "WebKit")
      LIST(APPEND QT_MODULES_WANTED Quick Multimedia Qml Sql Sensors Network Gui)

      # Depends on Positioning since Qt 5.3.0
      IF("${Qt5Core_VERSION_STRING}" VERSION_GREATER "5.2.9")
        LIST(APPEND QT_MODULES_WANTED Positioning)
      ENDIF()
    ELSEIF(_MODULE STREQUAL "WebKitWidgets")
      LIST(APPEND QT_MODULES_WANTED MultimediaWidgets OpenGL PrintSupport)
    ELSEIF(_MODULE STREQUAL "Multimedia")
      LIST(APPEND QT_MODULES_WANTED Gui Network)
    ELSEIF(_MODULE STREQUAL "MultimediaWidgets")
      LIST(APPEND QT_MODULES_WANTED Multimedia Widgets Gui OpenGL)
    ELSEIF(_MODULE STREQUAL "OpenGL")
      LIST(APPEND QT_MODULES_WANTED Widgets Gui)
    ELSEIF(_MODULE STREQUAL "PrintSupport")
      LIST(APPEND QT_MODULES_WANTED Widgets Gui)
    ELSEIF(_MODULE STREQUAL "Qml")
      LIST(APPEND QT_MODULES_WANTED Network)
    ELSEIF(_MODULE STREQUAL "Quick")
      LIST(APPEND QT_MODULES_WANTED Qml Network Gui)
    ENDIF()
  ENDFOREACH()

  LIST(REMOVE_DUPLICATES QT_MODULES_WANTED)
ENDMACRO()

MACRO(USE_QT_MODULES)
  OPTION(WITH_QT5 "Use Qt 5 instead of Qt 4" ON)

  SET(QT_MODULES_WANTED ${ARGN})
  SET(QT_MODULES_USED)
  SET(USE_QT OFF)
  SET(USE_QT4 OFF)
  SET(USE_QT5 OFF)

  # Qt shared modules
  SET(QT_SHARED_MODULES CLucene Core Declarative Gui Help Multimedia Network OpenGL Qml Script ScriptTools Sql Svg Test WebKit Xml XmlPatterns)

  # Qt 4 modules
  SET(QT4_MODULES ${QT_SHARED_MODULES} Main)

  # Qt 5 modules
  SET(QT5_MODULES ${QT_SHARED_MODULES} Bluetooth Concurrent LinguistTools MultimediaQuick MultimediaWidgets Nfc OpenGL OpenGLExtensions PlatformSupport Positioning PrintSupport Quick QuickParticles QuickTest QuickWidgets Sensors SerialPort V8 WebChannel WebKitWidgets WebSockets Widgets)

  # Use WinExtras only under Windows
  IF(WIN32)
    LIST(APPEND QT5_MODULES WinExtras)
  ENDIF()

  IF(WITH_QT5)
    # Look for Qt 5 in some environment variables
    SET(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${QTDIR} $ENV{QTDIR})
    FIND_PACKAGE(Qt5Core QUIET)

    IF(Qt5Core_FOUND)
      ADD_QT5_DEPENDENCIES()
      
      # To be sure a 2nd stage dependency is right
      ADD_QT5_DEPENDENCIES()

      SET(_ONLY_QT4 OFF)
      FOREACH(_MODULE ${QT_MODULES_WANTED})
        IF(_MODULE STREQUAL "QT4")
          SET(_ONLY_QT4 ON)
        ELSEIF(_MODULE STREQUAL "QT5")
          SET(_ONLY_QT4 OFF)
        ELSEIF(NOT _ONLY_QT4 AND QT5_MODULES MATCHES ${_MODULE})
          FIND_PACKAGE(Qt5${_MODULE} REQUIRED)
          IF("${Qt5${_MODULE}_FOUND}")
            LIST(APPEND QT_MODULES_USED ${_MODULE})
            SET(USE_QT5 ON)
            SET(USE_QT ON)
          ENDIF()
        ENDIF()
      ENDFOREACH()
    ELSE()
      SET(WITH_QT5 OFF)
    ENDIF()
  ENDIF()

  IF(NOT WITH_QT5)
    LIST(APPEND QT_MODULES_WANTED Main Core)
    LIST(REMOVE_DUPLICATES QT_MODULES_WANTED)

    SET(_ONLY_QT5 OFF)
    FOREACH(_MODULE ${QT_MODULES_WANTED})
      IF(_MODULE STREQUAL "QT5")
        SET(_ONLY_QT5 ON)
      ELSEIF(_MODULE STREQUAL "QT4")
        SET(_ONLY_QT5 OFF)
      ELSEIF(NOT _ONLY_QT5 AND QT4_MODULES MATCHES ${_MODULE})
        LIST(APPEND _COMPONENTS Qt${_MODULE})
      ENDIF()
    ENDFOREACH()
    FIND_PACKAGE(Qt4 COMPONENTS ${_COMPONENTS} REQUIRED)
    INCLUDE(${QT_USE_FILE})
    FOREACH(_MODULE ${QT_MODULES_WANTED})
      STRING(TOUPPER ${_MODULE} _UP_MODULE_NAME)
      IF("${QT_USE_QT${_UP_MODULE_NAME}}")
        LIST(APPEND QT_MODULES_USED ${_MODULE})
        SET(USE_QT4 ON)
        SET(USE_QT ON)
      ENDIF()
    ENDFOREACH()
  ENDIF()

  IF(USE_QT5)
    # Check if we are using Qt static or shared libraries
    GET_TARGET_PROPERTY(_FILE Qt5::Core IMPORTED_LOCATION_RELEASE)

    SET(_VERSION "${Qt5Core_VERSION_STRING}")
    
    IF(_FILE MATCHES "\\.(lib|a)$")
      SET(QT_STATIC ON)
      SET(_VERSION "${_VERSION} static version")
    ELSE()
      SET(QT_STATIC OFF)
      SET(_VERSION "${_VERSION} shared version")
    ENDIF()

    MESSAGE(STATUS "Found Qt ${_VERSION}")

    # These variables are not defined with Qt5 CMake modules
    SET(QT_BINARY_DIR "${_qt5Core_install_prefix}/bin")
    SET(QT_LIBRARY_DIR "${_qt5Core_install_prefix}/lib")
    SET(QT_PLUGINS_DIR "${_qt5Core_install_prefix}/plugins")
    SET(QT_TRANSLATIONS_DIR "${_qt5Core_install_prefix}/translations")

    # Fix wrong include directories with Qt 5 under Mac OS X
    INCLUDE_DIRECTORIES("${_qt5Core_install_prefix}/include")
  ENDIF()

  IF(USE_QT4)
    SET(QT_STATIC ${QT_IS_STATIC})
  ENDIF()
ENDMACRO()

MACRO(FILTER_QT_FILES FILE)
  IF(USE_QT)
    IF(${FILE} MATCHES "\\.ts$")
      STRING(REGEX REPLACE "^.*_([a-z-]*)\\.ts$" "\\1" _LANG ${FILE})
      LIST(APPEND QT_LANGS ${_LANG})
      LIST(APPEND QT_TSS ${FILE})
    ELSEIF(${FILE} MATCHES "\\.qrc$")
      LIST(APPEND QT_QRCS ${FILE})
    ELSEIF(${FILE} MATCHES "\\.ui$")
      LIST(APPEND QT_UIS ${FILE})
    ENDIF()
  ENDIF()
ENDMACRO()

MACRO(COMPILE_QT_RESOURCES)
  IF(QT_QRCS)
    # Generate .cpp from .qrc
    IF(USE_QT5)
      QT5_ADD_RESOURCES(QT_QRCS_CPPS ${QT_QRCS})
    ELSEIF(USE_QT4)
      QT4_ADD_RESOURCES(QT_QRCS_CPPS ${QT_QRCS})
    ENDIF()
  ENDIF()
ENDMACRO()

MACRO(COMPILE_QT_UIS)
  IF(QT_UIS)
    # Generate .h from .ui
    IF(USE_QT5)
      QT5_WRAP_UI(QT_UIS_HEADERS ${QT_UIS})
    ELSEIF(USE_QT4)
      QT4_WRAP_UI(QT_UIS_HEADERS ${QT_UIS})
    ENDIF()

    SOURCE_GROUP("ui" FILES ${QT_UIS})
  ENDIF()
ENDMACRO()

MACRO(COMPILE_QT_HEADERS _TARGET)
  IF(USE_QT)
    # CMake supports automoc since version 2.8.6
    IF(CMAKE_VERSION VERSION_GREATER "2.8.5" AND CMAKE_AUTOMOC)
      SET(QT_MOCS_CPPS "${CMAKE_CURRENT_BINARY_DIR}/${_TARGET}_automoc.cpp")
      SET_SOURCE_FILES_PROPERTIES(${QT_MOCS_CPPS} PROPERTIES GENERATED TRUE)
    ELSE()
      SET(_FILES "${ARGN}")
      IF(_FILES)
        # Generate .cpp from .h witout notice messages
        IF(USE_QT5)
          QT5_WRAP_CPP(QT_MOCS_CPPS ${_FILES} OPTIONS -nn)
        ELSEIF(USE_QT4)
          QT4_WRAP_CPP(QT_MOCS_CPPS ${_FILES})
        ENDIF()
      ENDIF()
    ENDIF()
    INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})
  ENDIF()
ENDMACRO()

MACRO(COMPILE_QT_TRANSLATIONS)
  IF(QT_TSS)
    SET_SOURCE_FILES_PROPERTIES(${QT_TSS} PROPERTIES OUTPUT_LOCATION "${CMAKE_BINARY_DIR}/translations")

    IF(WITH_UPDATE_TRANSLATIONS)
      SET(_TRANS ${ARGN} ${QT_UIS})
      IF(USE_QT5)
        QT5_CREATE_TRANSLATION(QT_QMS ${_TRANS} ${QT_TSS})
      ELSEIF(USE_QT4)
        QT4_CREATE_TRANSLATION(QT_QMS ${_TRANS} ${QT_TSS})
      ENDIF()
    ELSE()
      IF(USE_QT5)
        QT5_ADD_TRANSLATION(QT_QMS ${QT_TSS})
      ELSEIF(USE_QT4)
        QT4_ADD_TRANSLATION(QT_QMS ${QT_TSS})
      ENDIF()
    ENDIF()

    SOURCE_GROUP("translations" FILES ${QT_TSS})
  ENDIF()
ENDMACRO()

MACRO(SET_QT_SOURCES)
  IF(USE_QT)
    # Qt generated files
    SET(QT_SOURCES ${QT_MOCS_CPPS} ${QT_UIS_HEADERS} ${QT_QRCS_CPPS} ${QT_QMS})

    IF(QT_SOURCES)
      SOURCE_GROUP("generated" FILES ${QT_SOURCES})
      SET_SOURCES_FLAGS(${QT_SOURCES})
    ENDIF()
  ENDIF()
ENDMACRO()

MACRO(LINK_QT_LIBRARY _TARGET _NAME)
  IF(WIN32)
    SET(_PREFIX "Qt5")
    SET(_EXT "lib")
  ELSE()
    SET(_PREFIX "libQt5")
    SET(_EXT "a")
  ENDIF()
  SET(_LIB "${QT_LIBRARY_DIR}/${_PREFIX}${_NAME}.${_EXT}")
  IF(EXISTS ${_LIB})
    TARGET_LINK_LIBRARIES(${_TARGET} optimized ${_LIB})
  ENDIF()
  SET(_LIB "${QT_LIBRARY_DIR}/${_PREFIX}${_NAME}d.${_EXT}")
  IF(EXISTS ${_LIB})
    TARGET_LINK_LIBRARIES(${_TARGET} debug ${_LIB})
  ENDIF()
ENDMACRO()

MACRO(LINK_QT_PLUGIN _TARGET _TYPE _NAME)
  IF(WIN32)
    SET(_PREFIX "")
    SET(_EXT "lib")
  ELSE()
    SET(_PREFIX "lib")
    SET(_EXT "a")
  ENDIF()
  SET(_LIB "${QT_PLUGINS_DIR}/${_TYPE}/${_PREFIX}${_NAME}.${_EXT}")
  IF(EXISTS ${_LIB})
    TARGET_LINK_LIBRARIES(${_TARGET} optimized ${_LIB})
  ENDIF()
  SET(_LIB "${QT_PLUGINS_DIR}/${_TYPE}/${_PREFIX}${_NAME}d.${_EXT}")
  IF(EXISTS ${_LIB})
    TARGET_LINK_LIBRARIES(${_TARGET} debug ${_LIB})
  ENDIF()
ENDMACRO()

MACRO(LINK_QT_LIBRARIES _TARGET)
  IF(USE_QT)
    IF(USE_QT5)
      IF("${Qt5Core_VERSION_STRING}" VERSION_LESS "5.3" OR CMAKE_VERSION VERSION_LESS "2.8.12")
        QT5_USE_MODULES(${_TARGET} ${QT_MODULES_USED})
      ELSE()
        FOREACH(_MODULE ${QT_MODULES_USED})
          IF(TARGET "Qt5::${_MODULE}")
            TARGET_LINK_LIBRARIES(${_TARGET} "Qt5::${_MODULE}")
          ENDIF()
        ENDFOREACH()
      ENDIF()

      GET_TARGET_PROPERTY(_TYPE ${_TARGET} TYPE)

      # Link to QtMain
      IF(_TYPE STREQUAL "EXECUTABLE" AND CMAKE_VERSION VERSION_LESS "2.8.11")
        TARGET_LINK_LIBRARIES(${_TARGET} ${Qt5Core_QTMAIN_LIBRARIES})
      ENDIF()

      IF(QT_STATIC)
        ADD_DEFINITIONS(-DQT_STATICPLUGIN)

        FOREACH(_MODULE ${QT_MODULES_USED})
          IF(_MODULE STREQUAL "Core")
          ENDIF()
          IF(_MODULE STREQUAL "Network")
            FIND_PACKAGE(OpenSSL REQUIRED)
            FIND_PACKAGE(MyZLIB REQUIRED)
            TARGET_LINK_LIBRARIES(${_TARGET} ${OPENSSL_LIBRARIES} ${ZLIB_LIBRARIES})

            IF(WIN32)
              TARGET_LINK_LIBRARIES(${_TARGET}
                ${WINSDK_LIBRARY_DIR}/Crypt32.lib
                ${WINSDK_LIBRARY_DIR}/WS2_32.Lib)
            ENDIF()
          ENDIF()
          IF(_MODULE STREQUAL Gui)
            FIND_PACKAGE(MyPNG REQUIRED)
            FIND_PACKAGE(JPEG REQUIRED)

            TARGET_LINK_LIBRARIES(${_TARGET} ${PNG_LIBRARIES} ${JPEG_LIBRARIES})

            LINK_QT_LIBRARY(${_TARGET} PrintSupport)
            LINK_QT_LIBRARY(${_TARGET} PlatformSupport)

            IF(WIN32)
              TARGET_LINK_LIBRARIES(${_TARGET}
                ${WINSDK_LIBRARY_DIR}/Imm32.lib
                ${WINSDK_LIBRARY_DIR}/OpenGL32.lib
                ${WINSDK_LIBRARY_DIR}/WinMM.Lib)
              LINK_QT_PLUGIN(${_TARGET} platforms qwindows)
            ELSEIF(APPLE)
              FIND_LIBRARY(PCRE_LIBRARY pcre16 pcre)

              # Cups needs .dylib
              SET(OLD_CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES})
              SET(CMAKE_FIND_LIBRARY_SUFFIXES .dylib)
              FIND_LIBRARY(CUPS_LIBRARY cups)
              SET(CMAKE_FIND_LIBRARY_SUFFIXES ${OLD_CMAKE_FIND_LIBRARY_SUFFIXES})

              FIND_LIBRARY(IOKIT_FRAMEWORK IOKit)
              FIND_LIBRARY(FOUNDATION_FRAMEWORK Foundation)
              FIND_LIBRARY(COCOA_FRAMEWORK Cocoa)
              FIND_LIBRARY(CARBON_FRAMEWORK Carbon)
              FIND_LIBRARY(SYSTEMCONFIGURATION_FRAMEWORK SystemConfiguration)
              FIND_LIBRARY(SECURITY_FRAMEWORK Security)
              FIND_LIBRARY(OPENGL_FRAMEWORK NAMES OpenGL)

              TARGET_LINK_LIBRARIES(${_TARGET}
                ${CUPS_LIBRARY}
                ${PCRE_LIBRARY}
                ${FOUNDATION_FRAMEWORK}
                ${COCOA_FRAMEWORK}
                ${CARBON_FRAMEWORK}
                ${SYSTEMCONFIGURATION_FRAMEWORK}
                ${SECURITY_FRAMEWORK}
                ${IOKIT_FRAMEWORK}
                ${OPENGL_FRAMEWORK})

              LINK_QT_PLUGIN(${_TARGET} printsupport cocoaprintersupport)
              LINK_QT_PLUGIN(${_TARGET} platforms qcocoa)
            ENDIF()

            LINK_QT_PLUGIN(${_TARGET} imageformats qico)
            LINK_QT_PLUGIN(${_TARGET} imageformats qmng)

            # harfbuzz is needed since Qt 5.3
            IF(APPLE)
              SET(HB_LIB "${QT_LIBRARY_DIR}/libqtharfbuzzng.a")
            ELSEIF(WIN32)
              SET(HB_LIB "${QT_LIBRARY_DIR}/qtharfbuzzng.lib")
            ENDIF()
            IF(EXISTS ${HB_LIB})
              TARGET_LINK_LIBRARIES(${_TARGET} ${HB_LIB})
            ENDIF()
          ENDIF()
          IF(_MODULE STREQUAL "Multimedia")
            LINK_QT_PLUGIN(${_TARGET} mediaservice qtmedia_audioengine)

            IF(WIN32)
              LINK_QT_PLUGIN(${_TARGET} audio qtaudio_windows)
              LINK_QT_PLUGIN(${_TARGET} mediaservice dsengine)
              LINK_QT_PLUGIN(${_TARGET} mediaservice wmfengine)

              TARGET_LINK_LIBRARIES(${_TARGET} ${WINSDK_LIBRARY_DIR}/strmiids.lib)
            ELSEIF(APPLE)
              LINK_QT_PLUGIN(${_TARGET} audio qtaudio_coreaudio)

              FIND_LIBRARY(COREAUDIO_FRAMEWORK CoreAudio)
              FIND_LIBRARY(AUDIOUNIT_FRAMEWORK AudioUnit)
              FIND_LIBRARY(AUDIOTOOLBOX_FRAMEWORK AudioToolbox)

              TARGET_LINK_LIBRARIES(${_TARGET}
                ${AUDIOUNIT_FRAMEWORK}
                ${COREAUDIO_FRAMEWORK}
                ${AUDIOTOOLBOX_FRAMEWORK})
            ENDIF()
          ENDIF()
          IF(_MODULE STREQUAL "Widgets")
            LINK_QT_PLUGIN(${_TARGET} accessible qtaccessiblewidgets)
          ENDIF()
          IF(_MODULE STREQUAL "Sql")
            LINK_QT_PLUGIN(${_TARGET} sqldrivers qsqlite)
          ENDIF()
          IF(_MODULE STREQUAL "Svg")
            LINK_QT_PLUGIN(${_TARGET} imageformats qsvg)
            LINK_QT_PLUGIN(${_TARGET} iconengines qsvgicon)
          ENDIF()
        ENDFOREACH()
      ENDIF()
    ENDIF()
    IF(USE_QT4)
      TARGET_LINK_LIBRARIES(${_TARGET} ${QT_LIBRARIES})
      IF(APPLE)
        FIND_LIBRARY(SECURITY_FRAMEWORK Security)
        TARGET_LINK_LIBRARIES(${_TARGET} ${SECURITY_FRAMEWORK})
      ENDIF()
    ENDIF()
  ENDIF()
ENDMACRO()

MACRO(INSTALL_QT_TRANSLATIONS _TARGET)
  IF(QT_QMS)
    IF(APPLE)
      ADD_CUSTOM_COMMAND(TARGET ${_TARGET} PRE_BUILD COMMAND mkdir -p ${RESOURCES_DIR}/translations)

      # Copying all Qt translations to bundle
      FOREACH(_QM ${QT_QMS})
        ADD_CUSTOM_COMMAND(TARGET ${_TARGET} POST_BUILD COMMAND cp ARGS ${_QM} ${RESOURCES_DIR}/translations)
      ENDFOREACH()
    ELSE()
      # Install all applications Qt translations
      INSTALL(FILES ${QT_QMS} DESTINATION ${SHARE_PREFIX}/translations)
    ENDIF()

    IF(WIN32 OR APPLE)
      IF(USE_QT5)
        SET(_QM_BASE "qtbase")
      ELSEIF(USE_QT4)
        SET(_QM_BASE "qt")
      ENDIF()
      # Copy Qt standard translations
      FOREACH(_LANG ${QT_LANGS})
        SET(LANG_FILE "${QT_TRANSLATIONS_DIR}/${_QM_BASE}_${_LANG}.qm")
        IF(EXISTS ${LANG_FILE})
          IF(WIN32)
            INSTALL(FILES ${LANG_FILE} DESTINATION ${SHARE_PREFIX}/translations)
          ELSE()
            ADD_CUSTOM_COMMAND(TARGET ${_TARGET} POST_BUILD COMMAND cp ARGS ${LANG_FILE} ${RESOURCES_DIR}/translations)
          ENDIF()
        ENDIF()
      ENDFOREACH()
    ENDIF()
  ENDIF()
ENDMACRO()

MACRO(INSTALL_LIBRARY _NAME)
  IF(WIN32)
    SET(_PREFIX "")
    SET(_EXT "dll")
  ELSE()
    SET(_PREFIX "lib")
    SET(_EXT "so")
  ENDIF()
  SET(_LIB "${QT_BINARY_DIR}/${_PREFIX}${_NAME}.${_EXT}")
  IF(EXISTS ${_LIB})
    INSTALL(FILES ${_LIB} DESTINATION ${BIN_PREFIX})
  ENDIF()
ENDMACRO()

MACRO(INSTALL_QT_LIBRARY _NAME)
  IF(WIN32)
    SET(_PREFIX "Qt5")
    SET(_EXT "dll")
  ELSE()
    SET(_PREFIX "libQt5")
    SET(_EXT "so")
  ENDIF()
  SET(_LIB "${QT_BINARY_DIR}/${_PREFIX}${_NAME}.${_EXT}")
  IF(EXISTS ${_LIB})
    INSTALL(FILES ${_LIB} DESTINATION ${BIN_PREFIX})
  ENDIF()
ENDMACRO()

MACRO(INSTALL_QT_PLUGIN _TYPE _NAME)
  IF(WIN32)
    SET(_PREFIX "")
    SET(_EXT "dll")
  ELSE()
    SET(_PREFIX "lib")
    SET(_EXT "so")
  ENDIF()
  SET(_LIB "${QT_PLUGINS_DIR}/${_TYPE}/${_PREFIX}${_NAME}.${_EXT}")
  IF(EXISTS ${_LIB})
    INSTALL(FILES ${_LIB} DESTINATION ${BIN_PREFIX}/${_TYPE})
  ENDIF()
ENDMACRO()

MACRO(INSTALL_QT_LIBRARIES)
  IF(WIN32 AND USE_QT)
    IF(NOT QT_STATIC)
      # Install Qt libraries
      FOREACH(_MODULE ${QT_MODULES_USED})
        IF(USE_QT5)
          SET(_MODULE_NAME "Qt5${_MODULE}")
        ELSEIF(USE_QT4)
          SET(_MODULE_NAME "Qt${_MODULE}4")
        ENDIF()
        
        # Library
        IF(EXISTS ${QT_BINARY_DIR}/${_MODULE_NAME}.dll)
          INSTALL(FILES "${QT_BINARY_DIR}/${_MODULE_NAME}.dll" DESTINATION ${BIN_PREFIX})
        ENDIF()
        
        # Plugins
        IF(USE_QT4)
          IF(_MODULE STREQUAL "Gui")
            INSTALL_QT_PLUGIN(imageformats qgif4)
            INSTALL_QT_PLUGIN(imageformats qico4)
            INSTALL_QT_PLUGIN(imageformats qjpeg4)
          ENDIF()

          IF(_MODULE STREQUAL "Sql")
            INSTALL_QT_PLUGIN(sqldrivers qsqlite4)
          ENDIF()

          IF(_MODULE STREQUAL "Svg")
            INSTALL_QT_PLUGIN(imageformats qsvg4)
            INSTALL_QT_PLUGIN(iconengines qsvgicon4)
          ENDIF()
        ENDIF()

        IF(USE_QT5)
          IF(_MODULE STREQUAL "Core")
            INSTALL_LIBRARY(icuin51)
            INSTALL_LIBRARY(icuuc51)
            INSTALL_LIBRARY(icudt51)
            INSTALL_LIBRARY(icuin52)
            INSTALL_LIBRARY(icuuc52)
            INSTALL_LIBRARY(icudt52)
            INSTALL_QT_PLUGIN(platforms qwindows)
          ENDIF()

          IF(_MODULE STREQUAL "Gui")
            INSTALL_QT_PLUGIN(imageformats qico)
            INSTALL_QT_PLUGIN(imageformats qgif)
            INSTALL_QT_PLUGIN(imageformats qjpeg)
          ENDIF()

          IF(_MODULE STREQUAL "Widgets")
            INSTALL_QT_PLUGIN(accessible qtaccessiblewidgets)
          ENDIF()

          # Quick and Qml need V8
          IF(_MODULE MATCHES "^(Quick|Qml$)")
            INSTALL_QT_LIBRARY(V8)
          ENDIF()

          IF(_MODULE STREQUAL "Sql")
            INSTALL_QT_PLUGIN(sqldrivers qsqlite)
          ENDIF()

          IF(_MODULE STREQUAL "Svg")
            INSTALL_QT_PLUGIN(imageformats qsvg)
            INSTALL_QT_PLUGIN(iconengines qsvgicon)
          ENDIF()
        ENDIF()
      ENDFOREACH()

      # Install zlib DLL if found in Qt directory
      INSTALL_LIBRARY(zlib1)

      # Install OpenSSL libraries
      FOREACH(_ARG ${EXTERNAL_BINARY_PATH})
        IF(EXISTS "${_ARG}/libeay32.dll")
          INSTALL(FILES
            "${_ARG}/libeay32.dll"
            "${_ARG}/ssleay32.dll"
            DESTINATION ${BIN_PREFIX})
        ENDIF()
      ENDFOREACH()
    ENDIF()
  ENDIF()
ENDMACRO()

MACRO(INSTALL_QT_MISC _TARGET)
# Not needed anymore because qt_menu.nib is included in Qt resources
# IF(USE_QT4 AND QT_STATIC)
    # Copying qt_menu.nib to bundle
#    IF(APPLE AND MAC_RESOURCES_DIR)
#      ADD_CUSTOM_COMMAND(TARGET ${_TARGET} POST_BUILD COMMAND cp -R ARGS ${MAC_RESOURCES_DIR}/qt_menu.nib ${RESOURCES_DIR})
#    ENDIF()
#  ENDIF()
ENDMACRO()
