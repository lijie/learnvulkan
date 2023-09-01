cmake_minimum_required(VERSION 3.12)
project(learnvulkan)

#include_directories(${CMAKE_SOURCE_DIR}/deps/glm-0.9.9.9)
#include_directories(${CMAKE_SOURCE_DIR}/deps/assimp/include)

# add_library(toy 
# 	common/shader.cc common/camera.cc src/glad.c common/lo_common.cc common/shape.cc
# 	common/material.cc common/model.cc common/mesh.cc common/scene.cc common/cubemap.cc
# 	common/instance_renderer.cc common/directional_light.cc)
# target_include_directories(toy PRIVATE ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/common)

find_package(Vulkan)
include_directories(${Vulkan_INCLUDE_DIR})

include_directories(${CMAKE_SOURCE_DIR}/src)

# for w32
add_compile_definitions(VK_USE_PLATFORM_WIN32_KHR)

# ADD_LIBRARY(vulkan STATIC IMPORTED)
# SET_TARGET_PROPERTIES(vulkan PROPERTIES IMPORTED_LOCATION C:/VulkanSDK/1.3.250.1/Lib/vulkan-1.lib)
# include_directories(C:/VulkanSDK/1.3.250.1/Include)

add_library(base
	src/base/vulkan_device.cc src/base/vulkan_buffer.cc src/base/vulkan_initializers.cc src/base/vulkan_tools.cc src/base/vulkan_app.cc src/base/vulkan_debug.cc
	src/base/vulkan_swapchain.cc src/base/vulkan_pipelinebuilder.cc src/base/vertex_data.cc
)
target_include_directories(base PRIVATE ${CMAKE_SOURCE_DIR}/src/base)

if (true)
	ADD_LIBRARY(glfw STATIC IMPORTED)
	SET_TARGET_PROPERTIES(glfw PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/deps/glfw-3.3.8.bin.WIN64/lib-vc2022/glfw3.lib)
	include_directories(${CMAKE_SOURCE_DIR}/deps/glfw-3.3.8.bin.WIN64/include)

	include_directories(${CMAKE_SOURCE_DIR}/deps/stb)

	# ADD_LIBRARY(assimp STATIC IMPORTED)
	# SET_TARGET_PROPERTIES(assimp PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/deps/assimp/lib/assimp-vc142-mtd.lib)

	set(dep_libs glfw base Vulkan::Vulkan)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8 /std:c++20")
else ()
  set(dep_libs ${dep_libs_common} ${dep_libs_linux})
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
endif (true)

macro(add_vulkan_target)
	add_executable(${ARGV0} src/${ARGV0}/${ARGV0}.cc)
	target_include_directories(${ARGV0} PRIVATE ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/common)
	target_link_libraries(${ARGV0} LINK_PUBLIC ${dep_libs})
	# set_target_properties(${ARGV0} PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS /ENTRY:\"mainCRTStartup\"")
	# set_target_properties(${ARGV0} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/examples/runtime")
endmacro(add_vulkan_target)

macro(add_win32_vulkan_target)
	add_executable(${ARGV0} WIN32 src/${ARGV0}/${ARGV0}.cc)
	target_include_directories(${ARGV0} PRIVATE ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/common)
	target_link_libraries(${ARGV0} LINK_PUBLIC ${dep_libs})
	# set_target_properties(${ARGV0} PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS /ENTRY:\"mainCRTStartup\"")
	# set_target_properties(${ARGV0} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/examples/runtime")
endmacro(add_win32_vulkan_target)

add_vulkan_target(00-test)
add_vulkan_target(01-drawing-a-triangle)
add_vulkan_target(02-vertex-buffer)
add_vulkan_target(03-uniform-buffer)
add_vulkan_target(04-texture)
add_win32_vulkan_target(05-base-triangle)

