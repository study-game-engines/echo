CMAKE_MINIMUM_REQUIRED(VERSION 3.10)

SET(CMAKE_VERBOSE_MAKEFILE ON)

INCLUDE(CMakeDependentOption)

SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS On)

# options
OPTION(ECHO_BUILD_PLATFORM_ANDROID "Android platform." FALSE)

IF(UNIX)
	IF (APPLE)
		OPTION(ECHO_BUILD_PLATFORM_MAC "Mac platform." TRUE)
		OPTION(ECHO_BUILD_PLATFORM_IOS "Mac iOS platform." FALSE)
	ELSE()
		OPTION(ECHO_BUILD_PLATFORM_ANDROID "Android platform." TRUE)
	ENDIF()
ENDIF()

OPTION(ECHO_EDITOR_MODE "Editor Mode" TRUE)
OPTION(ECHO_RAYTRACING "Ray Tracing" FALSE)

OPTION(ECHO_GAME_SOURCE "With game source" FALSE)
SET(ECHO_GAME_NAME "game" CACHE STRING "Game name")
SET(ECHO_GAME_SOURCE_PATH "" CACHE PATH "Game source directory")

OPTION(THIRDPARTY_JPLAYER "JPlayer" FALSE)


IF(NOT ECHO_GAME_SOURCE)
	PROJECT(echo)
ELSE()
	PROJECT(${ECHO_GAME_NAME})	
ENDIF()

IF(ECHO_BUILD_PLATFORM_IOS AND ECHO_BUILD_PLATFORM_ANDROID)
	MESSAGE(FATAL_ERROR "Can only build for one platform.")
ENDIF()

SET(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1z")

IF(WIN32)
	IF(ECHO_BUILD_PLATFORM_ANDROID)
		ADD_DEFINITIONS("-DANDROID")

		SET(ECHO_PLATFORM_ANDROID TRUE)

		#notice the "recycling" of CMAKE_C_FLAGS
		#this is necessary to pick up android flags
		SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -pedantic -fPIC")

		MESSAGE(STATUS "Platform: Android")
	ELSEIF(ECHO_BUILD_PLATFORM_HTML5)
		SET(ECHO_PLATFORM_HTML5 TRUE)

		# enable c++11, memory growth
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1z")

		# enable memory growth
		SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -s ALLOW_MEMORY_GROWTH=1")
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s ALLOW_MEMORY_GROWTH=1")

		# enable exception catching
		SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -s DISABLE_EXCEPTION_CATCHING=0")
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s DISABLE_EXCEPTION_CATCHING=0")

	ELSE()
		SET(ECHO_PLATFORM_WINDOWS TRUE)
		MESSAGE(STATUS "Platform: Windows")

		# Mutil Thread Compiling
		SET(CMAKE_CXX_FLAGS "/DWIN32 /MP /EHsc")
	ENDIF()
ELSEIF(UNIX)
	IF(ECHO_BUILD_PLATFORM_ANDROID)

		SET(ECHO_PLATFORM_ANDROID TRUE)
		SET(CMAKE_OSX_SYSROOT "" CACHE STRING "" FORCE)
		SET(CMAKE_OSX_ARCHITECTURES "" CACHE STRING "" FORCE)
		SET(XCODE_ATTRIBUTE_SDKROOT "" CACHE STRING "" FORCE)

		#notice the "recycling" of CMAKE_C_FLAGS
		#this is necessary to pick up android flags
		SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -pedantic -fPIC")

		MESSAGE(STATUS "Platform: Android")

	ELSEIF(APPLE)
		INCLUDE(CMakeForceCompiler)
		SET(CMAKE_SIZEOF_VOID_P 4)
		SET(CMAKE_XCODE_ATTRIBUTE_GCC_VERSION "com.apple.compilers.llvm.clang.1_0")

		IF(ECHO_BUILD_PLATFORM_IOS)
			SET(ECHO_PLATFORM_IOS TRUE)
			MESSAGE(STATUS "Platform: Mac iOS")
		ELSE()
			SET(ECHO_PLATFORM_MAC TRUE)
			MESSAGE(STATUS "Platform: Mac OSX")
		ENDIF()

		IF(TRUE)
			SET(XCODE_ATTRIBUTE_GCC_SYMBOLS_PRIVATE_EXTERN "YES")

			IF(ECHO_PLATFORM_IOS)
				MESSAGE(STATUS "BUILD ECHO_PLATFORM_IOS")
        		SET(CMAKE_OSX_SYSROOT iphoneos CACHE STRING "" FORCE)
				SET(CMAKE_OSX_DEPLOYMENT_TARGET "")
				SET(CMAKE_EXE_LINKER_FLAGS "-framework Foundation -framework CoreGraphics -framework QuartzCore -framework UIKit")
				SET(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++")
				SET(CMAKE_XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET "11.0")
				SET(XCODE_ATTRIBUTE_SDKROOT iphoneos CACHE STRING "" FORCE)
				SET(XCODE_ATTRIBUTE_GCC_UNROLL_LOOPS "YES")
				SET(XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "iPhone Developer")
				SET(XCODE_ATTRIBUTE_GCC_PRECOMPILE_PREFIX_HEADER "YES")
				SET(XCODE_ATTRIBUTE_GCC_INLINES_ARE_PRIVATE_EXTERN "YES")
				SET(MACOSX_BUNDLE_GUI_IDENTIFIER "com.perfect.\${PRODUCT_NAME:rfc1034identifier}")
				SET(CMAKE_OSX_ARCHITECTURES ${ARCHS_STANDARD})
				REMOVE_DEFINITIONS(-msse)
			ELSE()
				SET(XCODE_ATTRIBUTE_SDKROOT macosx CACHE STRING "" FORCE)
				SET(CMAKE_OSX_SYSROOT macosx CACHE STRING "" FORCE)
				#SET(CMAKE_OSX_DEPLOYMENT_TARGET 9.0)
				IF (NOT CMAKE_OSX_ARCHITECTURES)
					SET(CMAKE_OSX_ARCHITECTURES ${ARCHS_STANDARD})
				ENDIF()

        		SET(XCODE_ATTRIBUTE_GCC_INLINES_ARE_PRIVATE_EXTERN[arch=x86_64] "YES")
			ENDIF()

		ENDIF()

	ELSE()
		SET(ECHO_PLATFORM_LINUX TRUE)
		MESSAGE(STATUS "Platform: Unix/Linux")
	ENDIF()
ELSE()
	MESSAGE(STATUS "Platform: Unknown")
ENDIF()

SET(ECHO_ROOT_PATH "${PROJECT_SOURCE_DIR}")

IF(ECHO_PLATFORM_WINDOWS)
	IF(CMAKE_SIZEOF_VOID_P EQUAL 8)
		SET(PLATFORM Win64)
	ELSE()
		SET(PLATFORM Win32)
	ENDIF()
ELSEIF(ECHO_PLATFORM_MAC)
	SET(PLATFORM Mac)
ENDIF()

# Set Lib and Bin base direoctory
IF(NOT ECHO_GAME_SOURCE)
	SET(ECHO_BIN_ROOT_PATH ${ECHO_ROOT_PATH}/bin/)
	SET(ECHO_LIB_ROOT_PATH ${ECHO_ROOT_PATH}/lib/)
ELSE()
	SET(ECHO_BIN_ROOT_PATH ${ECHO_GAME_SOURCE_PATH}/../Bin)
	SET(ECHO_LIB_ROOT_PATH ${ECHO_GAME_SOURCE_PATH}/../Lib)
ENDIF()

# Set library output path
IF(ECHO_PLATFORM_WINDOWS)
	SET(ECHO_LIB_PATH "${ECHO_LIB_ROOT_PATH}/${PLATFORM}/")
ELSEIF(ECHO_PLATFORM_MAC)
	SET(ECHO_LIB_PATH "${ECHO_LIB_ROOT_PATH}/OSX/${CMAKE_BUILD_TYPE}")
ELSEIF(ECHO_PLATFORM_LINUX)
	SET(ECHO_LIB_PATH "${ECHO_LIB_ROOT_PATH}/linux/")
ELSEIF(ECHO_PLATFORM_IOS)
	SET(ECHO_LIB_PATH "${ECHO_LIB_ROOT_PATH}/iOS/${CMAKE_BUILD_TYPE}")
ELSEIF(ECHO_PLATFORM_ANDROID)
	SET(ECHO_LIB_PATH "${ECHO_LIB_ROOT_PATH}/Android/${ANDROID_ABI}")
ELSEIF(ECHO_PLATFORM_HTML5)
	SET(ECHO_LIB_PATH "${ECHO_LIB_ROOT_PATH}/html5")
ENDIF()

IF(ECHO_EDITOR_MODE)
	SET(ECHO_BIN_PATH "${ECHO_BIN_ROOT_PATH}/editor/${PLATFORM}/${CMAKE_BUILD_TYPE}")
	SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${ECHO_BIN_PATH})
ELSE()
	SET(ECHO_BIN_PATH "${ECHO_BIN_ROOT_PATH}/app/${PLATFORM}/${CMAKE_BUILD_TYPE}")
	SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${ECHO_BIN_PATH})
ENDIF()

SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${ECHO_LIB_PATH}/${CMAKE_BUILD_TYPE}")
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${ECHO_LIB_PATH}/${CMAKE_BUILD_TYPE}")
SET(ECHO_DEP_PATH "${ECHO_ROOT_PATH}/thirdparty")

SET(ASSETS_OUT_DIR_CLASS_NAME "nonExistsClass")
SET(ASSETS_OUT_DIR_FUNCTION_NAME "getAssetsOutDir")
SET(ASSETS_OUT_DIR_SIGNATURE "()Ljava/lang/String;")

SET(APK_DIR_CLASS_NAME "nonExistsClass")
SET(APK_DIR_FUNCTION_NAME "getApkDir")
SET(APK_DIR_SIGNATURE "()Ljava/lang/String;")

MESSAGE(STATUS "\${ECHO_BUILD_PLATFORM_ANDROID}:${ECHO_BUILD_PLATFORM_ANDROID}")
MESSAGE(STATUS "\${ANDROID_ABI}:${ANDROID_ABI}")
MESSAGE(STATUS "\${ECHO_EDITOR_MODE}:${ECHO_EDITOR_MODE}")
MESSAGE(STATUS "\${CMAKE_BUILD_TYPE}: ${CMAKE_BUILD_TYPE}")
MESSAGE(STATUS "\${PROJECT_SOURCE_DIR}: ${PROJECT_SOURCE_DIR}")
MESSAGE(STATUS "\${PROJECT_BINARY_DIR}: ${PROJECT_BINARY_DIR}")
MESSAGE(STATUS "\${ECHO_ROOT_PATH}: ${ECHO_ROOT_PATH}")
MESSAGE(STATUS "\${ECHO_LIB_PATH}: ${ECHO_LIB_PATH}")
MESSAGE(STATUS "\${ECHO_BIN_PATH}: ${ECHO_BIN_PATH}")

# Qt
IF(${ECHO_PLATFORM_MAC})
	SET(QT_ROOT_PATH ${ECHO_ROOT_PATH}/editor/Qt/5.12.3/clang_64)
ELSEIF(${ECHO_PLATFORM_WINDOWS})
	SET(QT_ROOT_PATH ${ECHO_ROOT_PATH}/editor/Qt/5.10.0/msvc2017_64)
ENDIF()

# Tutorial - CMake Functions and Macros
# https://asitdhal.medium.com/cmake-functions-and-macros-22293041519f

# MACRO for GROUP_FILES
MACRO(GROUP_FILES src_files root_path)
        SET(cur_dir ${root_path})
        FOREACH(group_files ${${src_files}})
            STRING(REGEX REPLACE ${cur_dir}/\(.*\) \\1 sgbd_fpath ${group_files})
            STRING(REGEX REPLACE "\(.*\)/.*" \\1 sgbd_group_name ${sgbd_fpath})
            STRING(COMPARE EQUAL ${sgbd_fpath} ${sgbd_group_name} sgbd_nogroup)
            STRING(REPLACE "/" "\\" sgbd_group_name ${sgbd_group_name})
            IF(sgbd_nogroup)
				SOURCE_GROUP("" FILES ${group_files})
			ELSE()
				INCLUDE_DIRECTORIES(${sgbd_group_name})
				SOURCE_GROUP(${sgbd_group_name} FILES ${group_files})
            ENDIF(sgbd_nogroup)
        ENDFOREACH(group_files)
ENDMACRO(GROUP_FILES)

# Macrod Define for GROUP_FILES
MACRO(GROUP_FILES_NO_INCLUDE src_files root_path)
        SET(cur_dir ${root_path})
        FOREACH(group_files ${${src_files}})
            STRING(REGEX REPLACE ${cur_dir}/\(.*\) \\1 sgbd_fpath ${group_files})
            STRING(REGEX REPLACE "\(.*\)/.*" \\1 sgbd_group_name ${sgbd_fpath})
            STRING(COMPARE EQUAL ${sgbd_fpath} ${sgbd_group_name} sgbd_nogroup)
            STRING(REPLACE "/" "\\" sgbd_group_name ${sgbd_group_name})
            IF(sgbd_nogroup)
				SOURCE_GROUP("" FILES ${group_files})
			ELSE()
				SOURCE_GROUP(${sgbd_group_name} FILES ${group_files})
            ENDIF(sgbd_nogroup)
        ENDFOREACH(group_files)
ENDMACRO(GROUP_FILES_NO_INCLUDE)

MACRO(EXCLUDE_FILES src_files rescure exclude_dir)
	FILE(${rescure} excludefiles  ${exclude_dir})
	FOREACH(excludefile ${excludefiles})
		LIST(REMOVE_ITEM ${src_files} ${excludefile})
	ENDFOREACH(excludefile)
ENDMACRO(EXCLUDE_FILES)

MACRO(APPEND_FILES src_files rescure append_dir)
	FILE(${rescure} excludefiles  ${append_dir})

	MESSAGE(STATUS ${append_dir})
	MESSAGE(STATUS ${excludefiles})

	SET(${${src_files}} ${${src_files}} ${excludefiles})
	MESSAGE(STATUS ${${src_files}})
ENDMACRO(APPEND_FILES)

# Macro
MACRO(GROUP_QT_UI_FILES qt_generate_files src_files root_path)
		SET(cur_dir ${root_path})
		FOREACH(group_files ${${src_files}})
			STRING(REGEX REPLACE ${cur_dir}/\(.*\) \\1 sgbd_fpath ${group_files})
			STRING(REGEX REPLACE "\(.*\)/.*" \\1 sgbd_group_name ${sgbd_fpath})
			STRING(COMPARE EQUAL ${sgbd_fpath} ${sgbd_group_name} sgbd_nogroup)
			STRING(REPLACE "/" "\\" sgbd_group_name ${sgbd_group_name})
			IF(sgbd_nogroup)
				SOURCE_GROUP("" FILES ${group_files})
			ELSE()
				INCLUDE_DIRECTORIES(${sgbd_group_name})
				SOURCE_GROUP(${sgbd_group_name} FILES ${group_files})
				SET(QTUI_HEADER_UI)
				QT5_WRAP_UI(QTUI_HEADER_UI	${group_files})
				SOURCE_GROUP(${sgbd_group_name} FILES ${QTUI_HEADER_UI})
				SET(${qt_generate_files} ${${qt_generate_files}} ${QTUI_HEADER_UI})
			ENDIF(sgbd_nogroup)
		ENDFOREACH(group_files)
ENDMACRO(GROUP_QT_UI_FILES)

# define function GROUP_QT_MOC_FILES
FUNCTION(GROUP_QT_MOC_FILES qt_generate_files src_files)
	SET(QTUI_MOC_UI)
	QT5_WRAP_CPP(QTUI_MOC_UI ${${src_files}})
	SOURCE_GROUP("Resources\\Generated Files\\Moc" FILES ${QTUI_MOC_UI})
	SET(${qt_generate_files} ${${qt_generate_files}} ${QTUI_MOC_UI} PARENT_SCOPE)
ENDFUNCTION(GROUP_QT_MOC_FILES)

MACRO(TARGET_FIND_QTPACKAGES MODULE_NAME)
	IF(ECHO_EDITOR_MODE)
		IF(${ECHO_PLATFORM_MAC})
			SET(CMAKE_PREFIX_PATH ${QT_ROOT_PATH})
		ELSEIF(${ECHO_PLATFORM_WINDOWS})
			SET(CMAKE_PREFIX_PATH ${QT_ROOT_PATH})
		ENDIF()

		FIND_PACKAGE(Qt5 REQUIRED Widgets UiTools)
	ENDIF()
ENDMACRO(TARGET_FIND_QTPACKAGES)

MACRO(TARGET_LINK_QTLIBRARIES MODULE_NAME)
	iF(ECHO_EDITOR_MODE)
		TARGET_LINK_LIBRARIES(${MODULE_NAME} Qt5::Core Qt5::Widgets Qt5::UiTools)
	ENDIF()
ENDMACRO(TARGET_LINK_QTLIBRARIES)

# Add sub directories
ADD_SUBDIRECTORY(engine)
ADD_SUBDIRECTORY(thirdparty)

IF(ECHO_GAME_SOURCE)
	ADD_SUBDIRECTORY(${ECHO_GAME_SOURCE_PATH}/ ${PROJECT_BINARY_DIR}/game)
ENDIF()

# Editor mode
IF(ECHO_EDITOR_MODE)
	ADD_SUBDIRECTORY(tests/unittest)

	IF(ECHO_PLATFORM_WINDOWS)
		IF(CMAKE_SIZEOF_VOID_P EQUAL 8)
			ADD_SUBDIRECTORY("tool")
			ADD_SUBDIRECTORY("editor/echo")
		ELSE()
			MESSAGE(WARNING "Echo Editor only support x64 target on windows platform.")
		ENDIF()
	ELSEIF(ECHO_PLATFORM_MAC OR ECHO_PLATFORM_LINUX)
		ADD_SUBDIRECTORY("editor/echo")
	ENDIF()
ELSE()
	# All apps
	IF(ECHO_PLATFORM_WINDOWS)
		ADD_SUBDIRECTORY("app/windows")
	ELSEIF(ECHO_PLATFORM_MAC)
		ADD_SUBDIRECTORY("app/mac")
	ELSEIF(ECHO_PLATFORM_IOS)
		ADD_SUBDIRECTORY("app/ios")
	ELSEIF(ECHO_PLATFORM_ANDROID)
		ADD_SUBDIRECTORY("app/android/app/src/main/cpp/echo")
	ENDIF()
ENDIF()
