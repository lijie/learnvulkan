cmake_minimum_required(VERSION 3.12)
project(learnvulkan)

find_package(Vulkan)
include_directories(${Vulkan_INCLUDE_DIR})
set(GLSLC ${Vulkan_INCLUDE_DIR}/../Bin/glslc.exe)

include_directories(${CMAKE_SOURCE_DIR}/src)

# for w32
add_compile_definitions(VK_USE_PLATFORM_WIN32_KHR)
# for vsprintf
add_compile_definitions(_CRT_SECURE_NO_WARNINGS)

add_compile_definitions(GLM_FORCE_DEPTH_ZERO_TO_ONE)
add_compile_definitions(GLM_FORCE_RADIANS)
add_compile_definitions(GLM_ENABLE_EXPERIMENTAL)


# ADD_LIBRARY(vulkan STATIC IMPORTED)
# SET_TARGET_PROPERTIES(vulkan PROPERTIES IMPORTED_LOCATION C:/VulkanSDK/1.3.250.1/Lib/vulkan-1.lib)
# include_directories(C:/VulkanSDK/1.3.250.1/Include)

set(IMGUI_SOURCE
	deps/imgui/imgui_demo.cpp
	deps/imgui/imgui_draw.cpp
	deps/imgui/imgui_tables.cpp
	deps/imgui/imgui_widgets.cpp
	deps/imgui/imgui.cpp
)

add_library(base
	src/base/vulkan_device.cc src/base/vulkan_buffer.cc src/base/vulkan_initializers.cc src/base/vulkan_tools.cc src/base/vulkan_app.cc src/base/vulkan_debug.cc
	src/base/vulkan_swapchain.cc src/base/vulkan_pipelinebuilder.cc src/base/vertex_data.cc src/base/vulkan_texture.cc src/base/primitives.cc src/base/scene.cc
	src/base/vulkan_context.cc src/base/window.cc src/base/transform.cc src/base/camera.cc src/base/material.cc src/base/lvk_math.cc src/base/input.cc
	src/base/mesh_loader.cc src/base/directional_light.cc ${IMGUI_SOURCE}
)
target_include_directories(base PRIVATE ${CMAKE_SOURCE_DIR}/src/base)

if (true)
	ADD_LIBRARY(glfw STATIC IMPORTED)
	SET_TARGET_PROPERTIES(glfw PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/deps/glfw-3.3.8.bin.WIN64/lib-vc2022/glfw3.lib)
	include_directories(${CMAKE_SOURCE_DIR}/deps/glfw-3.3.8.bin.WIN64/include)

	include_directories(${CMAKE_SOURCE_DIR}/deps/stb)
	include_directories(${CMAKE_SOURCE_DIR}/deps/gltf)
	include_directories(${CMAKE_SOURCE_DIR}/deps/imgui)

	# ADD_LIBRARY(assimp STATIC IMPORTED)
	# SET_TARGET_PROPERTIES(assimp PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/deps/assimp/lib/assimp-vc142-mtd.lib)

	set(dep_libs glfw base Vulkan::Vulkan)
	#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8 /std:c++20")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20")
else ()
  set(dep_libs ${dep_libs_common} ${dep_libs_linux})
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
endif (true)

macro(add_shader_target)
	add_custom_command(
		OUTPUT ${CMAKE_SOURCE_DIR}/${ARGV2}
		COMMAND ${GLSLC} ${CMAKE_SOURCE_DIR}/${ARGV1} -o ${CMAKE_SOURCE_DIR}/${ARGV2}
	)
	add_custom_target(${ARGV0} DEPENDS ${CMAKE_SOURCE_DIR}/${ARGV2})
endmacro(add_shader_target)

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

macro(add_vulkan_target_with_shader)
	add_executable(${ARGV0} src/${ARGV0}/${ARGV0}.cc)
	target_include_directories(${ARGV0} PRIVATE ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/common)
	target_link_libraries(${ARGV0} LINK_PUBLIC ${dep_libs})
	add_shader_target(${ARGV0}_vs src/${ARGV0}/${ARGV0}.vert build/${ARGV0}.vert.spv)
	add_shader_target(${ARGV0}_ps src/${ARGV0}/${ARGV0}.frag build/${ARGV0}.frag.spv)
	add_dependencies(${ARGV0} ${ARGV0}_vs)
	add_dependencies(${ARGV0} ${ARGV0}_ps)
	# set_target_properties(${ARGV0} PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS /ENTRY:\"mainCRTStartup\"")
	# set_target_properties(${ARGV0} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/examples/runtime")
endmacro(add_vulkan_target_with_shader)

add_vulkan_target(00-test)
add_vulkan_target(01-drawing-a-triangle)
add_vulkan_target(02-vertex-buffer)
add_vulkan_target(03-uniform-buffer)
add_vulkan_target(04-texture)
add_vulkan_target(05-base-triangle)
add_vulkan_target(06-two-cubes)
add_vulkan_target(07-two-textures)
add_vulkan_target(08-move-camera)
add_vulkan_target(09-gltf)
add_vulkan_target_with_shader(10-pbr-basic)
add_vulkan_target_with_shader(11-pbr-ibl)

