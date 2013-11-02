cmake_minimum_required(VERSION 2.8)
project(resonant)

option(USE_QT5 NO)
set(SSE_LEVEL 2)
set(SOUND_API openal)
# set(ARCHITECTURE x86)
# set(SOUND_API opensl)

if(USE_QT5)
	add_definitions(-DQT5)
	message(STATUS "using Qt framework5...")
endif()

include(spinner/CMakeCommon.txt require)
include(spinner/CMakeCompiler.txt require)
if(SOUND_API STREQUAL "openal")
	message(STATUS "using OpenAL...")
	set(LIBSRC_FILE ${LIBSRC_FILE} sound_depAL.cpp)
	add_definitions(-DSOUND_HEADER="sound_depAL.hpp")
elseif(SOUND_API STREQUAL "opensl")
	message(STATUS "using OpenSL ES...")
	set(LIBSRC_FILE ${LIBSRC_FILE} sound_depSL.cpp)
	add_definitions(-DSOUND_HEADER="sound_depSL.hpp")
else()
	message(FATAL_ERROR, "unknown sound API")
endif()

include_directories(${PROJECT_SOURCE_DIR}/spinner
					${PROJECT_SOURCE_DIR}/boomstick
					${CMAKE_INSTALL_PREFIX}/include)
link_directories(${CMAKE_INSTALL_PREFIX}/lib_${TOOL_PREFIX})
if(DEFINED ARCHITECTURE)
	add_definitions(-DGL_GLEXT_PROTOTYPES -DUSE_OPENGLES2)
	include_directories(${ANDROID_NDK_ROOT}/sources/android/support/include
						${CMAKE_INSTALL_PREFIX}/include/SDL2)
else()
	include_directories(/usr/local/include/SDL2)
endif()
#  gltexture.cpp
set(LIBSRC_FILE ${LIBSRC_FILE} glresource.cpp gpu.cpp glx_parse2.cpp glx_parse.cpp glx.cpp glx_output.cpp glshader.cpp glframebuffer.cpp glformat.cpp error.cpp glbuffer.cpp adaptsdl.cpp sound.cpp acommon.cpp sdlw_rwops.cpp vorbisfile.cpp looper.cpp sdltouchpad.cpp sdlmouse.cpp sdlkeyboard.cpp sdljoypad.cpp input.cpp sdlw_glctx.cpp sdlw_window.cpp sdlw.cpp sdlw_mutex.cpp sdlw_spec.cpp)
add_library(resonant STATIC ${LIBSRC_FILE})
install(TARGETS resonant ARCHIVE DESTINATION lib_${TOOL_PREFIX})

file(GLOB HEADERS "./*.hpp")
install(FILES ${HEADERS} DESTINATION include/resonant)

if(NOT DEFINED ARCHITECTURE)
	set(SRC_FILE ${SRC_FILE} main.cpp)
	add_executable(rse_test ${SRC_FILE})
	target_link_libraries(rse_test resonant boomstick spinner SDL2 openal vorbisfile boost_system)
	install(TARGETS rse_test RUNTIME DESTINATION bin_${TOOL_PREFIX})
endif()
