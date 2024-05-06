cmake_minimum_required(VERSION 3.20)
project(t85play LANGUAGES CXX)

include(FetchContent)
FetchContent_Declare(t85apu_emu
	GIT_REPOSITORY https://github.com/ADM228/ATtiny85APU.git
	GIT_TAG main)

set(T85APU_REGWRITE_BUFFER_SIZE 1)
FetchContent_MakeAvailable(t85apu_emu)

add_executable(t85play t85play.cpp)
target_compile_features(t85play PRIVATE cxx_std_17)
	

set(BUILD_SHARED_LIBS FALSE)

target_link_libraries(t85play PRIVATE t85apu_emu sndfile soundio)