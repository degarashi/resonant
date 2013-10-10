cmake_minimum_required(VERSION 2.8)
project(resonant)
set(CMAKE_CXX_COMPILER "clang++")
include_directories(${PROJECT_SOURCE_DIR}
					${PROJECT_SOURCE_DIR}/spinner
					${PROJECT_SOURCE_DIR}/boomstick)
include_directories(/usr/local/include/c++/4.8.1
					/usr/local/include/c++/4.8.1/i686-pc-linux-gnu)
link_directories("/usr/local/lib/gcc/i686-pc-linux-gnu/4.8.1")
set(CMAKE_CXX_FLAGS_DEBUG "-DDEBUG -std=c++11 -ggdb3 -O0 -msse2")
set(CMAKE_CXX_FLAGS_RELEASE "-std=c++11 -O2 -msse2")
set(CMAKE_BUILD_TYPE "Debug")
link_directories(/tmp/spinner_build
				/tmp/boomstick_build)
add_executable(rse_test sound.cpp sound_depAL.cpp acommon.cpp sdlw_rwops.cpp vorbisfile.cpp looper.cpp sdltouchpad.cpp sdlmouse.cpp sdlkeyboard.cpp sdljoypad.cpp input.cpp sdlw_glctx.cpp sdlw_window.cpp sdlw.cpp sdlw_mutex.cpp sdlw_spec.cpp main.cpp)
target_link_libraries(rse_test spinner boomstick boost_system SDL2 openal vorbisfile)

install(TARGETS rse_test RUNTIME DESTINATION bin)
