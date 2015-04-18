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

# OUTPUT_DIR
# CONTENTS_DIR
# RESOURCES_DIR
# MAC_RESOURCES_DIR
# MAC_LANGS

MACRO(SIGN_FILE_MAC _TARGET)
  IF(IOS)
    IF(NOT IOS_DEVELOPER)
      SET(IOS_DEVELOPER "iPhone Developer")
    ENDIF()
    IF(NOT IOS_DISTRIBUTION)
      SET(IOS_DISTRIBUTION "${IOS_DEVELOPER}")
    ENDIF()
    SET_TARGET_PROPERTIES(${_TARGET} PROPERTIES
      XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY[variant=Debug] ${IOS_DEVELOPER}
      XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY[variant=Release] ${IOS_DISTRIBUTION}
      XCODE_ATTRIBUTE_INSTALL_PATH "$(LOCAL_APPS_DIR)"
      XCODE_ATTRIBUTE_INSTALL_PATH_VALIDATE_PRODUCT "YES"
      XCODE_ATTRIBUTE_COMBINE_HIDPI_IMAGES "NO")
  ELSE()
#   SET_TARGET_PROPERTIES(${target} PROPERTIES
#     XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "Mac Developer")
  ENDIF()
ENDMACRO()

MACRO(INIT_MAC)
  SET(MAC_LANGS)
  SET(MAC_RESOURCES_fr)
  SET(MAC_RESOURCES_en)
  SET(MAC_RESOURCES_de)
  SET(MAC_RESOURCES_es)
  SET(MAC_RESOURCES_it)
  SET(MAC_RESOURCES_NEUTRAL)
  SET(MAC_FRAMEWORKS)
  SET(MAC_XIBS)
  SET(MAC_STRINGS)
  SET(MAC_ICNSS)
  SET(MAC_INFO_PLIST)
  SET(MAC_ITUNESARTWORK)
  SET(MAC_ITUNESARTWORK2X)
  SET(MAC_MOBILEPRIVISION)
  
  # Regex filter for Mac files
  SET(MAC_FILES_FILTER "(\\.(xib|strings|icns|plist|framework|mobileprovision)|iTunesArtwork.*\\.png|\\.lproj/.*)$")
ENDMACRO()

MACRO(FILTER_MAC_FILES FILE)
  IF(APPLE)
    IF(${FILE} MATCHES "\\.xib$")
      LIST(APPEND MAC_XIBS ${FILE})
      IF(NOT XCODE)
        # Don't include XIB with makefiles because we only need NIB
        SET(_INCLUDE OFF)
      ENDIF()
    ELSEIF(${FILE} MATCHES "\\.strings$")
      LIST(APPEND _STRINGS ${FILE})
    ELSEIF(${FILE} MATCHES "embedded-xcode\\.mobileprovision$")
        SET(_INCLUDE OFF)
        IF(XCODE)
          SET(MAC_MOBILEPRIVISION ${FILE})
        ENDIF()
    ELSEIF(${FILE} MATCHES "embedded\\.mobileprovision$")
        SET(_INCLUDE OFF)
        IF(NOT XCODE)
          SET(MAC_MOBILEPRIVISION ${FILE})
        ENDIF()
    ELSEIF(${FILE} MATCHES "iTunesArtwork\\.png$")
        # Don't include iTunesArtwork because it'll be copied in IPA
        SET(_INCLUDE OFF)
        SET(MAC_ITUNESARTWORK ${FILE})
    ELSEIF(${FILE} MATCHES "iTunesArtwork@2x\\.png$")
        # Don't include iTunesArtwork@2x because it'll be copied in IPA
        SET(_INCLUDE OFF)
        SET(MAC_ITUNESARTWORK2X ${FILE})
    ELSEIF(${FILE} MATCHES "\\.icns$")
      LIST(APPEND MAC_ICNSS ${FILE})
    ELSEIF(${FILE} MATCHES "Info([a-z0-9_-]*)\\.plist$")
      # Don't include Info.plist because it'll be generated
      LIST(APPEND MAC_INFO_PLIST ${FILE})
      SET(_INCLUDE OFF)
    ELSEIF(${FILE} MATCHES "\\.framework$")
      LIST(APPEND MAC_FRAMEWORKS ${FILE})
      SET(_INCLUDE OFF)
    ENDIF()

    IF(${FILE} MATCHES "/([a-z]+)\\.lproj/")
      # Extract ISO code for language from source directory
      STRING(REGEX REPLACE "^.*/([a-z]+)\\.lproj/.*$" "\\1" _LANG ${FILE})

      # Append new language if not already in the list
      LIST(FIND MAC_LANGS "${_LANG}" _INDEX)
      IF(_INDEX EQUAL -1)
        LIST(APPEND MAC_LANGS ${_LANG})
      ENDIF()

      # Append file to localized resources list
      LIST(APPEND MAC_RESOURCES_${_LANG} ${FILE})
    ELSE()
      # Append file to neutral resources list
      IF(_INCLUDE)
        LIST(APPEND MAC_RESOURCES_NEUTRAL ${FILE})
      ENDIF()
    ENDIF()
  ENDIF()
ENDMACRO()

MACRO(INIT_BUNDLE _TARGET)
    IF(XCODE)
      SET(OUTPUT_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$(CONFIGURATION)/${PRODUCT_FIXED}.app)
    ELSE()
      SET(OUTPUT_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${PRODUCT_FIXED}.app)
    ENDIF()

    IF(NOT IOS)
      SET(_SUBDIR "mac")
      SET(CONTENTS_DIR ${OUTPUT_DIR}/Contents)
      SET(RESOURCES_DIR ${CONTENTS_DIR}/Resources)
    ELSE()
      SET(_SUBDIR "ios")
      SET(CONTENTS_DIR ${OUTPUT_DIR})
      SET(RESOURCES_DIR ${CONTENTS_DIR})
    ENDIF()

    IF(NOT MAC_RESOURCES_DIR)
      FOREACH(ITEM ${CMAKE_MODULE_PATH})
        IF(EXISTS "${ITEM}/${_SUBDIR}/Info.plist")
          SET(MAC_RESOURCES_DIR "${ITEM}/${_SUBDIR}")
          BREAK()
        ENDIF()
      ENDFOREACH()
    ENDIF()

    IF(NOT MACOSX_BUNDLE_INFO_STRING)
      SET(MACOSX_BUNDLE_INFO_STRING "${PRODUCT}")
    ENDIF()

    IF(NOT MACOSX_BUNDLE_LONG_VERSION_STRING)
      SET(MACOSX_BUNDLE_LONG_VERSION_STRING "${PRODUCT} version ${VERSION}")
    ENDIF()

    IF(NOT MACOSX_BUNDLE_BUNDLE_NAME)
      SET(MACOSX_BUNDLE_BUNDLE_NAME "${PRODUCT}")
    ENDIF()

    IF(NOT MACOSX_BUNDLE_SHORT_VERSION_STRING)
      SET(MACOSX_BUNDLE_SHORT_VERSION_STRING ${VERSION})
    ENDIF()

    IF(NOT MACOSX_BUNDLE_BUNDLE_VERSION)
      SET(MACOSX_BUNDLE_BUNDLE_VERSION ${VERSION})
    ENDIF()

    IF(NOT MACOSX_BUNDLE_COPYRIGHT)
      SET(MACOSX_BUNDLE_COPYRIGHT "Copyright ${YEAR} ${AUTHOR}")
    ENDIF()
    
    IF(NOT CPACK_BUNDLE_NAME)
      SET(CPACK_BUNDLE_NAME "${PRODUCT_FIXED}")
    ENDIF()

    # Make sure the 'Resources' Directory is correctly created before we build
    ADD_CUSTOM_COMMAND(TARGET ${_TARGET} PRE_BUILD COMMAND mkdir -p ${RESOURCES_DIR})
ENDMACRO()

MACRO(SET_TARGET_FLAGS_XCODE _TARGET)
  IF(XCODE)
    IF(IOS AND IOS_VERSION)
      SET_TARGET_PROPERTIES(${_TARGET} PROPERTIES
        XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET ${IOS_VERSION}
        XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"
        XCODE_ATTRIBUTE_VALID_ARCHS "armv7") # armv6 armv7 armv7s
    ENDIF()

    IF(WITH_VISIBILITY_HIDDEN)
      SET_TARGET_PROPERTIES(${_TARGET} PROPERTIES
        XCODE_ATTRIBUTE_GCC_SYMBOLS_PRIVATE_EXTERN YES
        XCODE_ATTRIBUTE_GCC_INLINES_ARE_PRIVATE_EXTERN YES)
    ENDIF()

    IF(NOT WITH_EXCEPTIONS)
      SET_TARGET_PROPERTIES(${_TARGET} PROPERTIES XCODE_ATTRIBUTE_GCC_ENABLE_CPP_EXCEPTIONS NO)
    ENDIF()

    IF(NOT WITH_RTTI)
      SET_TARGET_PROPERTIES(${_TARGET} PROPERTIES XCODE_ATTRIBUTE_GCC_ENABLE_CPP_RTTI NO)
    ENDIF()
  ENDIF()
ENDMACRO()

MACRO(INIT_BUILD_FLAGS_MAC)
  IF(APPLE)
    IF(IOS)
      # Disable CMAKE_OSX_DEPLOYMENT_TARGET for iOS
      SET(CMAKE_OSX_DEPLOYMENT_TARGET "" CACHE PATH "" FORCE)
    ENDIF()

    IF(XCODE)
      IF(IOS)
        SET(CMAKE_OSX_SYSROOT "iphoneos" CACHE PATH "" FORCE)
      ENDIF()
    ELSE()
      IF(CMAKE_OSX_ARCHITECTURES)
        SET(TARGETS_COUNT 0)
        SET(_ARCHS)
        FOREACH(_ARCH ${CMAKE_OSX_ARCHITECTURES})
          IF(_ARCH STREQUAL "i386")
            SET(_ARCHS "${_ARCHS} i386")
            SET(TARGET_X86 1)
            MATH(EXPR TARGETS_COUNT "${TARGETS_COUNT}+1")
          ELSEIF(_ARCH STREQUAL "x86_64")
            SET(_ARCHS "${_ARCHS} x86_64")
            SET(TARGET_X64 1)
            MATH(EXPR TARGETS_COUNT "${TARGETS_COUNT}+1")
          ELSEIF(_ARCH STREQUAL "armv7s")
            SET(_ARCHS "${_ARCHS} armv7s")
            SET(TARGET_ARMV7S 1)
            SET(TARGET_ARM 1)
            MATH(EXPR TARGETS_COUNT "${TARGETS_COUNT}+1")
          ELSEIF(_ARCH STREQUAL "armv7")
            SET(_ARCHS "${_ARCHS} armv7")
            SET(TARGET_ARMV7 1)
            SET(TARGET_ARM 1)
            MATH(EXPR TARGETS_COUNT "${TARGETS_COUNT}+1")
          ELSEIF(_ARCH STREQUAL "armv6")
            SET(_ARCHS "${_ARCHS} armv6")
            SET(TARGET_ARMV6 1)
            SET(TARGET_ARM 1)
            MATH(EXPR TARGETS_COUNT "${TARGETS_COUNT}+1")
          ELSEIF(_ARCH STREQUAL "mips")
            SET(_ARCHS "${_ARCHS} mips")
            SET(TARGET_MIPS 1)
            MATH(EXPR TARGETS_COUNT "${TARGETS_COUNT}+1")
          ELSE()
            SET(_ARCHS "${_ARCHS} unknwon(${_ARCH})")
          ENDIF()
        ENDFOREACH()
        MESSAGE(STATUS "Compiling under Mac OS X for ${TARGETS_COUNT} architectures: ${_ARCHS}")
      ELSE()
        SET(TARGETS_COUNT 0)
      ENDIF()

      IF(TARGETS_COUNT EQUAL 1)
        IF(TARGET_ARM)
          IF(TARGET_ARMV7S)
            ADD_PLATFORM_FLAGS("-arch armv7s -DHAVE_ARMV7S")
          ENDIF()

          IF(TARGET_ARMV7)
            ADD_PLATFORM_FLAGS("-arch armv7 -DHAVE_ARMV7")
          ENDIF()

          IF(TARGET_ARMV6)
            ADD_PLATFORM_FLAGS("-arch armv6 -DHAVE_ARMV6")
          ENDIF()

          IF(TARGET_ARMV5)
            ADD_PLATFORM_FLAGS("-arch armv5 -DHAVE_ARMV5")
          ENDIF()

          ADD_PLATFORM_FLAGS("-mthumb -DHAVE_ARM")
        ENDIF()

        IF(TARGET_X64)
          ADD_PLATFORM_FLAGS("-arch x86_64 -DHAVE_X64 -DHAVE_X86_64 -DHAVE_X86")
        ELSEIF(TARGET_X86)
          ADD_PLATFORM_FLAGS("-arch i386 -DHAVE_X86")
        ENDIF()

        IF(TARGET_MIPS)
          ADD_PLATFORM_FLAGS("-arch mips -DHAVE_MIPS")
        ENDIF()
      ELSEIF(TARGETS_COUNT EQUAL 0)
        # Not using CMAKE_OSX_ARCHITECTURES, HAVE_XXX already defined before
        IF(TARGET_ARM)
          IF(TARGET_ARMV7S)
            ADD_PLATFORM_FLAGS("-arch armv7s")
          ENDIF()

          IF(TARGET_ARMV7)
            ADD_PLATFORM_FLAGS("-arch armv7")
          ENDIF()

          IF(TARGET_ARMV6)
            ADD_PLATFORM_FLAGS("-arch armv6")
          ENDIF()

          IF(TARGET_ARMV5)
            ADD_PLATFORM_FLAGS("-arch armv5")
          ENDIF()

          ADD_PLATFORM_FLAGS("-mthumb")
        ENDIF()

        IF(TARGET_X64)
          ADD_PLATFORM_FLAGS("-arch x86_64")
        ELSEIF(TARGET_X86)
          ADD_PLATFORM_FLAGS("-arch i386")
        ENDIF()

        IF(TARGET_MIPS)
          ADD_PLATFORM_FLAGS("-arch mips")
        ENDIF()
      ELSE()
        IF(TARGET_ARMV6)
          ADD_PLATFORM_FLAGS("-Xarch_armv6 -mthumb -Xarch_armv6 -DHAVE_ARM -Xarch_armv6 -DHAVE_ARMV6")
        ENDIF()

        IF(TARGET_ARMV7)
          ADD_PLATFORM_FLAGS("-Xarch_armv7 -mthumb -Xarch_armv7 -DHAVE_ARM -Xarch_armv7 -DHAVE_ARMV7")
        ENDIF()

        IF(TARGET_ARMV7S)
          ADD_PLATFORM_FLAGS("-Xarch_armv7s -mthumb -Xarch_armv7s -DHAVE_ARM -Xarch_armv7s -DHAVE_ARMV7 -Xarch_armv7s -DHAVE_ARMV7S")
        ENDIF()

        IF(TARGET_X86)
          ADD_PLATFORM_FLAGS("-Xarch_i386 -DHAVE_X86")
        ENDIF()

        IF(TARGET_X64)
          ADD_PLATFORM_FLAGS("-Xarch_x86_64 -DHAVE_X64 -Xarch_x86_64 -DHAVE_X86_64")
        ENDIF()

        IF(TARGET_MIPS)
          ADD_PLATFORM_FLAGS("-Xarch_mips -DHAVE_MIPS")
        ENDIF()
      ENDIF()

      IF(IOS)
        SET(CMAKE_OSX_SYSROOT "" CACHE PATH "" FORCE)

        IF(IOS_VERSION)
          PARSE_VERSION_STRING(${IOS_VERSION} IOS_VERSION_MAJOR IOS_VERSION_MINOR IOS_VERSION_PATCH)
          CONVERT_VERSION_NUMBER(IOS_VERSION_NUMBER 10 ${IOS_VERSION_MAJOR} ${IOS_VERSION_MINOR} ${IOS_VERSION_PATCH})

          ADD_PLATFORM_FLAGS("-D__IPHONE_OS_VERSION_MIN_REQUIRED=${IOS_VERSION_NUMBER}")
        ENDIF()

        IF(CMAKE_IOS_SYSROOT)
          IF(TARGET_ARMV7S)
            IF(TARGETS_COUNT GREATER 1)
              SET(XARCH "-Xarch_armv7s ")
            ENDIF()

            ADD_PLATFORM_FLAGS("${XARCH}-isysroot${CMAKE_IOS_SYSROOT}")
            ADD_PLATFORM_FLAGS("${XARCH}-miphoneos-version-min=${IOS_VERSION}")
            SET(PLATFORM_LINKFLAGS "${PLATFORM_LINKFLAGS} ${XARCH}-Wl,-iphoneos_version_min,${IOS_VERSION}")
          ENDIF()

          IF(TARGET_ARMV7)
            IF(TARGETS_COUNT GREATER 1)
              SET(XARCH "-Xarch_armv7 ")
            ENDIF()

            ADD_PLATFORM_FLAGS("${XARCH}-isysroot${CMAKE_IOS_SYSROOT}")
            ADD_PLATFORM_FLAGS("${XARCH}-miphoneos-version-min=${IOS_VERSION}")
            SET(PLATFORM_LINKFLAGS "${PLATFORM_LINKFLAGS} ${XARCH}-Wl,-iphoneos_version_min,${IOS_VERSION}")
          ENDIF()

          IF(TARGET_ARMV6)
            IF(TARGETS_COUNT GREATER 1)
              SET(XARCH "-Xarch_armv6 ")
            ENDIF()

            ADD_PLATFORM_FLAGS("${XARCH}-isysroot${CMAKE_IOS_SYSROOT}")
            ADD_PLATFORM_FLAGS("${XARCH}-miphoneos-version-min=${IOS_VERSION}")
            SET(PLATFORM_LINKFLAGS "${PLATFORM_LINKFLAGS} ${XARCH}-Wl,-iphoneos_version_min,${IOS_VERSION}")
          ENDIF()
        ENDIF()

        IF(CMAKE_IOS_SIMULATOR_SYSROOT)
          IF(TARGET_X64)
            IF(TARGETS_COUNT GREATER 1)
              SET(XARCH "-Xarch_x86_64 ")
            ENDIF()

            ADD_PLATFORM_FLAGS("${XARCH}-isysroot${CMAKE_IOS_SIMULATOR_SYSROOT}")
            ADD_PLATFORM_FLAGS("${XARCH}-mios-simulator-version-min=${IOS_VERSION}")
            SET(PLATFORM_LINKFLAGS "${PLATFORM_LINKFLAGS} ${XARCH}-Wl,-macosx_version_min,${CMAKE_OSX_DEPLOYMENT_TARGET}")
          ENDIF()

          IF(TARGET_X86)
            IF(TARGETS_COUNT GREATER 1)
              SET(XARCH "-Xarch_i386 ")
            ENDIF()

            ADD_PLATFORM_FLAGS("${XARCH}-isysroot${CMAKE_IOS_SIMULATOR_SYSROOT}")
            ADD_PLATFORM_FLAGS("${XARCH}-mios-simulator-version-min=${IOS_VERSION}")
            SET(PLATFORM_LINKFLAGS "${PLATFORM_LINKFLAGS} ${XARCH}-Wl,-macosx_version_min,${CMAKE_OSX_DEPLOYMENT_TARGET}")
          ENDIF()
        ENDIF()
      ELSE()
        # Always force -mmacosx-version-min to override environement variable
        # __MAC_OS_X_VERSION_MIN_REQUIRED
        SET(PLATFORM_LINKFLAGS "${PLATFORM_LINKFLAGS} -Wl,-macosx_version_min,${CMAKE_OSX_DEPLOYMENT_TARGET}")
      ENDIF()
    ENDIF()
  ENDIF()
ENDMACRO()

MACRO(INSTALL_MAC_RESOURCES _TARGET)
  # Copy all resources in Resources folder
  IF(MAC_RESOURCES_NEUTRAL OR MISC_FILES)
    SOURCE_GROUP(Resources FILES ${MAC_RESOURCES_NEUTRAL} ${MISC_FILES})
    IF(XCODE)
      # Resources are copied by Xcode
      SET_SOURCE_FILES_PROPERTIES(${MAC_RESOURCES_NEUTRAL} ${MISC_FILES} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    ELSE()
      # We need to copy resources manually
      FOREACH(_RES ${MAC_RESOURCES_NEUTRAL})
        ADD_CUSTOM_COMMAND(TARGET ${_TARGET} PRE_BUILD COMMAND cp ARGS ${_RES} ${RESOURCES_DIR})
      ENDFOREACH()
    ENDIF()
  ENDIF()

  IF(MAC_LANGS)
    FOREACH(_LANG ${MAC_LANGS})
      # Create the directory containing specific language resources
      ADD_CUSTOM_COMMAND(TARGET ${_TARGET} PRE_BUILD COMMAND mkdir -p ${RESOURCES_DIR}/${_LANG}.lproj)
      SOURCE_GROUP("Resources\\${_LANG}.lproj" FILES ${MAC_RESOURCES_${_LANG}})
      FOREACH(_RES ${MAC_RESOURCES_${_LANG}})
        ADD_CUSTOM_COMMAND(TARGET ${_TARGET} PRE_BUILD COMMAND cp ARGS ${_RES} ${RESOURCES_DIR}/${_LANG}.lproj)
      ENDFOREACH()
    ENDFOREACH()
  ENDIF()

  # Set a custom plist file for the app bundle
  IF(MAC_RESOURCES_DIR)
    IF(NOT MAC_INFO_PLIST)
      SET(MAC_INFO_PLIST ${MAC_RESOURCES_DIR}/Info.plist)
    ENDIF()

    SET_TARGET_PROPERTIES(${_TARGET} PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${MAC_INFO_PLIST})
    SET(CPACK_BUNDLE_PLIST ${MAC_INFO_PLIST})

    IF(NOT XCODE)
      ADD_CUSTOM_COMMAND(TARGET ${_TARGET} POST_BUILD COMMAND cp ARGS ${MAC_RESOURCES_DIR}/PkgInfo ${CONTENTS_DIR})
    ENDIF()
  ENDIF()

  IF(MAC_ICNSS)
    # Copying all icons to bundle
    FOREACH(_ICNS ${MAC_ICNSS})
      # If target Mac OS X, use first icon for Bundle
      IF(NOT IOS AND NOT MACOSX_BUNDLE_ICON_FILE)
        GET_FILENAME_COMPONENT(_ICNS_NAME ${_ICNS} NAME)
        SET(MACOSX_BUNDLE_ICON_FILE ${_ICNS_NAME})
      ENDIF()
      IF(NOT CPACK_BUNDLE_ICON)
        SET(CPACK_BUNDLE_ICON ${_ICNS})
        SET(CPACK_PACKAGE_ICON ${_ICNS})
      ENDIF()
      IF(NOT XCODE)
        ADD_CUSTOM_COMMAND(TARGET ${_TARGET} POST_BUILD COMMAND cp ARGS ${_ICNS} ${RESOURCES_DIR})
      ENDIF()
    ENDFOREACH()
  ENDIF()

  IF(MISC_FILES AND NOT XCODE)
    # Copying all misc files to bundle
    FOREACH(_MISC ${MISC_FILES})
      ADD_CUSTOM_COMMAND(TARGET ${_TARGET} POST_BUILD COMMAND cp ARGS ${_MISC} ${RESOURCES_DIR})
    ENDFOREACH()
  ENDIF()

  # Copying .mobileprovision file
  IF(MAC_MOBILEPRIVISION)
    ADD_CUSTOM_COMMAND(TARGET ${_TARGET} POST_BUILD COMMAND cp ARGS ${MAC_MOBILEPRIVISION} ${RESOURCES_DIR}/embedded.mobileprovision)
  ENDIF()
ENDMACRO()

MACRO(INSTALL_RESOURCES_MAC _TARGET _DIR)
  IF(APPLE)
    IF(_DIR)
      ADD_CUSTOM_COMMAND(TARGET ${_TARGET} PRE_BUILD COMMAND cp -R ARGS ${_DIR}/ ${RESOURCES_DIR})
    ENDIF()

    IF(CPACK_PACKAGE_DESCRIPTION_FILE)
      ADD_CUSTOM_COMMAND(TARGET ${_TARGET} PRE_BUILD COMMAND cp ARGS ${CPACK_PACKAGE_DESCRIPTION_FILE} ${RESOURCES_DIR})
    ENDIF()
    
    IF(CPACK_RESOURCE_FILE_LICENSE)
      ADD_CUSTOM_COMMAND(TARGET ${_TARGET} PRE_BUILD COMMAND cp ARGS ${CPACK_RESOURCE_FILE_LICENSE} ${RESOURCES_DIR})
    ENDIF()
  ENDIF()
ENDMACRO()

MACRO(COMPILE_MAC_XIBS _TARGET)
  # Compile the .xib files using the 'ibtool' program with the destination being the app package
  IF(MAC_XIBS)
    IF(NOT XCODE)
      # Make sure we can find the 'ibtool' program. If we can NOT find it we skip generation of this project
      FIND_PROGRAM(IBTOOL ibtool HINTS "/usr/bin" "${OSX_DEVELOPER_ROOT}/usr/bin" NO_CMAKE_FIND_ROOT_PATH)

      IF(${IBTOOL} STREQUAL "IBTOOL-NOTFOUND")
        MESSAGE(SEND_ERROR "ibtool can not be found and is needed to compile the .xib files. It should have been installed with the Apple developer tools. The default system paths were searched in addition to ${OSX_DEVELOPER_ROOT}/usr/bin")
      ENDIF()

      FOREACH(XIB ${MAC_XIBS})
        IF(XIB MATCHES "\\.lproj")
          STRING(REGEX REPLACE "^.*/(([a-z]+)\\.lproj/([a-zA-Z0-9_-]+))\\.xib$" "\\1.nib" NIB ${XIB})
        ELSE()
          STRING(REGEX REPLACE "^.*/([a-zA-Z0-9_-]+)\\.xib$" "\\1.nib" NIB ${XIB})
        ENDIF()
        GET_FILENAME_COMPONENT(NIB_OUTPUT_DIR ${RESOURCES_DIR}/${NIB} PATH)
        ADD_CUSTOM_COMMAND(TARGET ${_TARGET} POST_BUILD
          COMMAND ${IBTOOL} --errors --warnings --notices --output-format human-readable-text
            --compile ${RESOURCES_DIR}/${NIB}
            ${XIB}
            --sdk ${CMAKE_IOS_SDK_ROOT}
          COMMENT "Building XIB object ${NIB}")
      ENDFOREACH()
    ENDIF()
  ENDIF()
ENDMACRO()

MACRO(FIX_IOS_BUNDLE _TARGET)
  # Fixing Bundle files for iOS
  IF(IOS)
    ADD_CUSTOM_COMMAND(TARGET ${_TARGET} POST_BUILD
      COMMAND mv ${OUTPUT_DIR}/Contents/MacOS/* ${OUTPUT_DIR}
      COMMAND mv ${OUTPUT_DIR}/Contents/Info.plist ${OUTPUT_DIR}
      COMMAND rm -rf ${OUTPUT_DIR}/Contents)

    # Adding other needed files
    ADD_CUSTOM_COMMAND(TARGET ${_TARGET} POST_BUILD
      COMMAND cp ARGS ${CMAKE_IOS_SDK_ROOT}/ResourceRules.plist ${CONTENTS_DIR})
  ENDIF()
ENDMACRO()

MACRO(CREATE_IOS_PACKAGE_TARGET _TARGET)
  IF(IOS)
    # Creating .ipa package
    IF(MAC_ITUNESARTWORK)
      IF(APPSTORE)
        CONFIGURE_FILE(${MAC_RESOURCES_DIR}/application_store.xcent ${CMAKE_BINARY_DIR}/application.xcent)
      ELSE()
        CONFIGURE_FILE(${MAC_RESOURCES_DIR}/application.xcent ${CMAKE_BINARY_DIR}/application.xcent)
      ENDIF()

      IF(NOT PACKAGE_NAME)
        SET(PACKAGE_NAME "${PRODUCT_FIXED}")
      ENDIF()
      
      SET(IPA_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${PACKAGE_NAME}_ipa)
      SET(IPA ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${PACKAGE_NAME}-${VERSION}.ipa)

      SET(_COMMANDS)

      IF(MAC_ITUNESARTWORK2X)
        LIST(APPEND _COMMANDS COMMAND cp "${MAC_ITUNESARTWORK2X}" "${IPA_DIR}/iTunesArtwork@2x")
      ENDIF()

      ADD_CUSTOM_TARGET(packages
        COMMAND rm -rf "${OUTPUT_DIR}/Contents"
        COMMAND mkdir -p "${IPA_DIR}/Payload"
        COMMAND strip "${CONTENTS_DIR}/${PRODUCT_FIXED}"
        COMMAND security unlock-keychain -p "${KEYCHAIN_PASSWORD}"
        COMMAND CODESIGN_ALLOCATE=${CMAKE_IOS_DEVELOPER_ROOT}/usr/bin/codesign_allocate codesign -fs "${IOS_DISTRIBUTION}" "--resource-rules=${CONTENTS_DIR}/ResourceRules.plist" --entitlements "${CMAKE_BINARY_DIR}/application.xcent" "${CONTENTS_DIR}"
        COMMAND cp -R "${OUTPUT_DIR}" "${IPA_DIR}/Payload"
        COMMAND cp "${MAC_ITUNESARTWORK}" "${IPA_DIR}/iTunesArtwork"
        ${_COMMANDS}
        COMMAND ditto -c -k "${IPA_DIR}" "${IPA}"
        COMMAND rm -rf "${IPA_DIR}"
        COMMENT "Creating IPA archive..."
        SOURCES ${MAC_ITUNESARTWORK}
        VERBATIM)
      ADD_DEPENDENCIES(packages ${_TARGET})
      SET_TARGET_LABEL(packages "PACKAGE")
    ENDIF()
  ENDIF()
ENDMACRO()

MACRO(CREATE_MAC_PACKAGE_TARGET _TARGET)
  IF(APPLE AND NOT IOS)
    # Creating .ipa package
    SET(_APP ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${PRODUCT_FIXED}.app)
    SET(_PKG ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${_TARGET}-${VERSION}-osx.pkg)

    ADD_CUSTOM_TARGET(packages
      COMMAND productbuild --identifier "${MACOSX_BUNDLE_GUI_IDENTIFIER}" --version "${VERSION}" --component ${_APP} /Applications ${_PKG}
      COMMENT "Creating OS X package..."
      SOURCES ${MAC_ITUNESARTWORK}
      VERBATIM)
    ADD_DEPENDENCIES(packages ${_TARGET})
    SET_TARGET_LABEL(packages "PACKAGE")
  ENDIF()
ENDMACRO()

MACRO(CREATE_IOS_RUN_TARGET _TARGET)
  IF(IOS AND NOT IOS_PLATFORM STREQUAL "OS")
    SET(IOS_SIMULATOR "${CMAKE_IOS_DEVELOPER_ROOT}/Applications/iPhone Simulator.app/Contents/MacOS/iPhone Simulator")
    IF(EXISTS ${IOS_SIMULATOR})
      ADD_CUSTOM_TARGET(run
        COMMAND rm -rf ${OUTPUT_DIR}/Contents
        COMMAND ${IOS_SIMULATOR} -SimulateApplication ${OUTPUT_DIR}/${PRODUCT_FIXED}
        COMMENT "Launching iOS simulator...")
      ADD_DEPENDENCIES(run ${_TARGET})
      SET_TARGET_LABEL(run "RUN")
    ENDIF()
  ENDIF()
ENDMACRO()
