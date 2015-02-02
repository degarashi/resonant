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

include_directories(${PROJECT_SOURCE_DIR}/spinner
					${PROJECT_SOURCE_DIR}/boomstick
					${CMAKE_INSTALL_PREFIX}/include
					${CMAKE_INSTALL_PREFIX}/include/SDL
					${CMAKE_INSTALL_PREFIX}/include/lua)
link_directories(${CMAKE_INSTALL_PREFIX}/lib_${TOOL_PREFIX})
if(DEFINED ARCHITECTURE)
	# Android環境
	add_definitions(-DINCLUDE_OPENGL_ANDROID
					-DGL_GLEXT_PROTOTYPES
					-DUSE_OPENGLES2)
	include_directories(${ANDROID_NDK_ROOT}/sources/android/support/include
						${CMAKE_INSTALL_PREFIX}/include/SDL2)
elseif(${WIN32})
	# Windows環境
	#TODO OpenGL関数インクルードの定義
	execute_process(COMMAND which ${CMAKE_CXX_COMPILER} OUTPUT_VARIABLE CompilerPath)
	execute_process(COMMAND ${PROJECT_SOURCE_DIR}/getbasepath.sh ${CompilerPath} OUTPUT_VARIABLE CompilerPath)
	execute_process(COMMAND ${PROJECT_SOURCE_DIR}/getbasepath.sh ${CompilerPath} OUTPUT_VARIABLE CompilerPath)
	set(CompilerBasePath ${CompilerPath}/i686-w64-mingw32)
	include_directories(${CompilerBasePath}/include/SDL2
						${CompilerBasePath}/include/freetype2)
else()
	# Linux環境
	add_definitions(-DINCLUDE_OPENGL_1_X
					-DINCLUDE_OPENGL_2_X
					-DINCLUDE_OPENGL_3_X
					-DINCLUDE_OPENGL_FRAMEBUFFER
					-DINCLUDE_OPENGL_SYNC
					-DINCLUDE_OPENGL_TIMER_QUERY)
	include_directories(/usr/local/include/SDL2
						/usr/local/include/freetype2)
endif()
add_library(resonant STATIC ${LIBSRC_FILE} ${DEPSRC_FILE})
install(TARGETS resonant ARCHIVE DESTINATION lib_${TOOL_PREFIX})

file(GLOB HEADERS "./*.hpp")
install(FILES ${HEADERS} DESTINATION include/resonant)
if(NOT DEFINED ARCHITECTURE)
	if(${WIN32})
		add_definitions(-Dmain=SDL_main)
		set(LINK_LIBS resonant boomstick spinner mingw32 SDL2_image SDL2main SDL2
			freetype opengl32 OpenAL32 vorbisfile vorbis ogg boost_system boost_regex boost_serialization png z lua)
	else()
		set(LINK_LIBS resonant boomstick spinner SDL2_image SDL2 freetype vorbisfile ogg boost_system boost_regex boost_serialization GL openal lua dl z)
	endif()

	# -------- 動作テスト --------
	aux_source_directory(test/ SRC_FILE)
	add_executable(rse ${SRC_FILE})
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
