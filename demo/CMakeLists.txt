cmake_minimum_required(VERSION 3.7)

project(C64MON)

# Set to TRUE to use tracy profiler framework
set(PROFILING FALSE)

if(PROFILING)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DTRACY_ENABLE=1")
	set(PROFILE_SOURCES "../tracy/public/TracyClient.cpp")
endif()

find_package(SDL2 REQUIRED)
if(UNIX)
	message(STATUS "using unix")
	find_package(PkgConfig REQUIRED)
	pkg_search_module(SDL2_IMAGE REQUIRED SDL2_image)
	pkg_search_module(SDL2_MIXER REQUIRED SDL2_mixer)
	find_package(Threads)
else()
	find_package(SDL2_image REQUIRED)
	find_package(SDL2_mixer REQUIRED)
endif()
find_package(OpenGL REQUIRED)

file(GLOB_RECURSE DEMO_SRC "*.cpp" "*.c")

file(GLOB IMGUI_CORE_SRC "../imgui/*.cpp")
set(IMGUI_BACKEND_SRC "../imgui/backends/imgui_impl_sdl.cpp" "../imgui/backends/imgui_impl_opengl2.cpp")
set(IMGUI_SRC ${IMGUI_CORE_SRC} ${IMGUI_BACKEND_SRC})

include_directories("../imgui/" "../imgui/backends" ${SDL2_INCLUDE_DIRS} "../tracy/public")

add_executable(c64mon "main.cpp" ${DEMO_SRC} ${IMGUI_SRC} ${PROFILE_SOURCES})

target_link_libraries(c64mon ${SDL2_LIBRARIES} ${SDL2_IMAGE_LIBRARIES} ${OPENGL_LIBRARIES} ${SDL2_MIXER_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
