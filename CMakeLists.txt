cmake_minimum_required(VERSION 2.8)
project(resonant)

# Linux: BUILD_TYPE = Linux
# MinGW: BUILD_TYPE = MinGW
# Android:	BUILD_TYPE = AndroidX86 or AndroidArm
include("crosscompile/${BUILD_TYPE}.cmake" require)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-unused-function")

# この辺は実際に使うOpenGLのバージョンに合わせて定義する
add_definitions(-DTOOL_PREFIX="${TOOL_PREFIX}")

# 依存ファイル以外のソースファイルを収集
file(GLOB LIBSRC_FILE RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.cpp)
file(GLOB LIBSRC_FILE_UTIL RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "util/*.cpp")
list(APPEND LIBSRC_FILE ${LIBSRC_FILE_UTIL})
list(REMOVE_ITEM LIBSRC_FILE main.cpp sound_depAL.cpp sound_depSL.cpp)
if(SOUND_API STREQUAL "openal")
	message(STATUS "using OpenAL...")
	set(DEPSRC_FILE ${DEPSRC_FILE} sound_depAL.cpp)
	add_definitions(-DSOUND_HEADER="sound_depAL.hpp")
elseif(SOUND_API STREQUAL "opensl")
	message(STATUS "using OpenSL ES...")
	set(DEPSRC_FILE ${DEPSRC_FILE} sound_depSL.cpp)
	add_definitions(-DSOUND_HEADER="sound_depSL.hpp")
else()
	message(FATAL_ERROR, "unknown sound API")
endif()

find_package(Boost 1.55 REQUIRED system regex serialization thread)
find_package(Threads REQUIRED)
find_package(ZLIB REQUIRED)
find_package(Lua 5.3.1 REQUIRED)
find_package(OpenGL 3 REQUIRED)
include_directories(
	${PROJECT_SOURCE_DIR}/spinner
	${PROJECT_SOURCE_DIR}/boomstick
	${CMAKE_INSTALL_PREFIX}/include
	${CMAKE_INSTALL_PREFIX}/include/SDL
	${Boost_INCLUDE_DIRS}
	${LUA_INCLUDE_DIRS}
	${ZLIB_INCLUDE_DIRS}
)
if(NOT DEFINED ARCHITECTURE)
	find_package(OpenAL REQUIRED)
	find_package(Freetype 2.0 REQUIRED)
	include_directories(
		${OPENAL_INCLUDE_DIRS}
		${FREETYPE_INCLUDE_DIRS}
	)
endif()

link_directories(${CMAKE_INSTALL_PREFIX}/lib_${TOOL_PREFIX})
if(DEFINED ARCHITECTURE)
	# Android環境
	add_definitions(-DINCLUDE_OPENGL_ANDROID
					-DGL_GLEXT_PROTOTYPES
					-DUSE_OPENGLES2)
	include_directories(
		${ANDROID_NDK_ROOT}/sources/android/support/include
		${CMAKE_INSTALL_PREFIX}/include/SDL2
	)
elseif(${WIN32})
	# Windows環境
	#TODO OpenGL関数インクルードの定義
	execute_process(COMMAND which ${CMAKE_CXX_COMPILER} OUTPUT_VARIABLE CompilerPath)
	execute_process(COMMAND ${PROJECT_SOURCE_DIR}/getbasepath.sh ${CompilerPath} OUTPUT_VARIABLE CompilerPath)
	execute_process(COMMAND ${PROJECT_SOURCE_DIR}/getbasepath.sh ${CompilerPath} OUTPUT_VARIABLE CompilerPath)
	set(CompilerBasePath ${CompilerPath}/i686-w64-mingw32)
	include_directories(
		${CompilerBasePath}/include/SDL2
	)
else()
	# Linux環境
	add_definitions(-DINCLUDE_OPENGL_1_X
					-DINCLUDE_OPENGL_2_X
					-DINCLUDE_OPENGL_3_X
					-DINCLUDE_OPENGL_FRAMEBUFFER
					-DINCLUDE_OPENGL_SYNC
					-DINCLUDE_OPENGL_TIMER_QUERY)
	include_directories(
		/usr/local/include/SDL2
	)
endif()
add_library(resonant STATIC ${LIBSRC_FILE} ${DEPSRC_FILE})
install(TARGETS resonant ARCHIVE DESTINATION lib_${TOOL_PREFIX})

file(GLOB_RECURSE HEADERS "*.hpp")
# 除外するヘッダディレクトリを指定
set(COPYHEADERS_EXCLUDE_DIR spinner boomstick glextract test unittest)
set(COPYHEADERS_MY_DIR "resonant")
include(crosscompile/CopyHeaders.cmake require)

file(GLOB HEADERS "glext.h")
install(FILES ${HEADERS} DESTINATION include/resonant)

file(GLOB INCLUDES "./opengl_define/*.inc")
install(FILES ${INCLUDES} DESTINATION include/resonant/opengl_define)
if(NOT DEFINED ARCHITECTURE)
	if(${WIN32})
		add_definitions(-Dmain=SDL_main)
		set(LINK_LIBS
			resonant
			boomstick
			spinner
			mingw32
			SDL2_image
			SDL2main
			SDL2
			${FREETYPE_LIBRARIES}
			${OPENGL_LIBRARIES}
			OpenAL32
			vorbisfile
			vorbis
			ogg
			${Boost_LIBRARIES}
			png
			${ZLIB_LIBRARIES}
			${LUA_LIBRARIES}
		)
	else()
		find_package(GTest REQUIRED)
		include_directories(${GTEST_INCLUDE_DIRS})
		set(LINK_LIBS
			resonant
			boomstick
			spinner
			SDL2_image
			SDL2
			${FREETYPE_LIBRARIES}
			${Boost_LIBRARIES}
			${GTEST_LIBRARIES}
			${CMAKE_THREAD_LIBS_INIT}
			${LUA_LIBRARIES}
			${GTEST_LIBRARIES}
			vorbisfile
			ogg
			${OPENGL_LIBRARIES}
			openal
			dl
			${ZLIB_LIBRARIES}
		)
	endif()

	# -------- 動作テスト --------
	aux_source_directory(test/ SRC_FILE)
	aux_source_directory(test/tweak/ SRC_FILE2)
	add_executable(rse ${SRC_FILE} ${SRC_FILE2})
	target_link_libraries(rse ${LINK_LIBS})
	install(TARGETS rse RUNTIME DESTINATION bin_${TOOL_PREFIX})

	# -------- 単体テスト --------
	aux_source_directory(unittest/ UNITSRC_FILE)
	add_executable(rse_test ${UNITSRC_FILE})
	target_link_libraries(rse_test ${LINK_LIBS} gtest pthread)
	install(TARGETS rse_test RUNTIME DESTINATION bin_${TOOL_PREFIX})
	add_test(NAME All
			WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
			COMMAND rse_test)
	enable_testing()
endif()
