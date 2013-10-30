cmake_minimum_required(VERSION 2.8)
project(resonant)

set(SSE_LEVEL 2)
set(SOUND_API openal)
include(spinner/CMakePC.txt require)
include(spinner/CMakeCompiler.txt require)
if(SOUND_API STREQUAL "openal")
	message(STATUS "using OpenAL...")
	set(LIBSRC_FILE ${LIBSRC_FILE} sound_depAL.cpp)
elseif(SOUND_API STREQUAL "opensl")
	message(STATUS "using OpenSL ES...")
	set(LIBSRC_FILE ${LIBSRC_FILE} sound_depSL.cpp)
else()
	message(FATAL_ERROR, "unknown sound API")
endif()

include_directories(${PROJECT_SOURCE_DIR}/spinner
					${PROJECT_SOURCE_DIR}/boomstick
					/usr/local/include/SDL2)
link_directories(${CMAKE_INSTALL_PREFIX}/lib_${TOOL_PREFIX})

set(LIBSRC_FILE ${LIBSRC_FILE} adaptsdl.cpp sound.cpp acommon.cpp sdlw_rwops.cpp vorbisfile.cpp looper.cpp sdltouchpad.cpp sdlmouse.cpp sdlkeyboard.cpp sdljoypad.cpp input.cpp sdlw_glctx.cpp sdlw_window.cpp sdlw.cpp sdlw_mutex.cpp sdlw_spec.cpp)
add_library(resonant STATIC ${LIBSRC_FILE})

set(SRC_FILE ${SRC_FILE} main.cpp)
add_executable(rse_test ${SRC_FILE})
target_link_libraries(rse_test boomstick spinner SDL2 openal vorbisfile boost_system)

install(TARGETS resonant rse_test
	ARCHIVE DESTINATION lib_${TOOL_PREFIX}
	RUNTIME DESTINATION bin_${TOOL_PREFIX})
file(GLOB HEADERS "./*.hpp")
install(FILES ${HEADERS} DESTINATION include/resonant)
