cmake_minimum_required(VERSION 3.12)
project(learnvulkan)

#include_directories(${CMAKE_SOURCE_DIR}/deps/glm-0.9.9.9)
#include_directories(${CMAKE_SOURCE_DIR}/deps/assimp/include)


# add_library(toy 
# 	common/shader.cc common/camera.cc src/glad.c common/lo_common.cc common/shape.cc
# 	common/material.cc common/model.cc common/mesh.cc common/scene.cc common/cubemap.cc
# 	common/instance_renderer.cc common/directional_light.cc)
# target_include_directories(toy PRIVATE ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/common)

if (true)
	ADD_LIBRARY(glfw STATIC IMPORTED)
	SET_TARGET_PROPERTIES(glfw PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/deps/glfw-3.3.8.bin.WIN64/lib-vc2022/glfw3.lib)
	include_directories(${CMAKE_SOURCE_DIR}/deps/glfw-3.3.8.bin.WIN64/include)

    ADD_LIBRARY(vulkan STATIC IMPORTED)
    SET_TARGET_PROPERTIES(vulkan PROPERTIES IMPORTED_LOCATION C:/VulkanSDK/1.3.250.1/Lib/vulkan-1.lib)
    include_directories(C:/VulkanSDK/1.3.250.1/Include)

	# ADD_LIBRARY(assimp STATIC IMPORTED)
	# SET_TARGET_PROPERTIES(assimp PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/deps/assimp/lib/assimp-vc142-mtd.lib)

	set(dep_libs glfw vulkan)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8 /std:c++17")
else ()
  set(dep_libs ${dep_libs_common} ${dep_libs_linux})
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
endif (true)

macro(add_vulkan_target)
	add_executable(${ARGV0} src/${ARGV0}/${ARGV0}.cc)
	target_include_directories(${ARGV0} PRIVATE ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/common)
	target_link_libraries(${ARGV0} LINK_PUBLIC ${dep_libs})
	# set_target_properties(${ARGV0} PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS /ENTRY:\"mainCRTStartup\"")
	set_target_properties(${ARGV0} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/examples/runtime")
endmacro(add_vulkan_target)

add_vulkan_target(00-test)
add_vulkan_target(01-drawing-a-triangle)
# add_opengl_target(color_triangle)
# add_opengl_target(texture)
# add_opengl_target(transformation)
# add_opengl_target(coordinate)
# add_opengl_target(camera)
# add_opengl_target(basic_lighting)
# add_opengl_target(basic_model)
# add_opengl_target(stencil_test)
# add_opengl_target(blending_test)
# add_opengl_target(framebuffer_test)
# add_opengl_target(cubemap_test)
# add_opengl_target(antialiasing_test)
# add_opengl_target(instancing_test)
# add_opengl_target(phong_test)
