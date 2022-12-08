# 3.11.0 required for cxx_std_17 and FetchContent
cmake_minimum_required(VERSION 3.11.0 FATAL_ERROR)
project(ChipiVersary VERSION 0.0.1 LANGUAGES C CXX)

# Sets the CMake build dirs to the Snobaste standard.
include(OneLibrary/CMake/SnobasteIncludeDirs.cmake)

# Allows usage of configure time content
include(FetchContent)

set(SNOBASTE_AUDIO ON CACHE INTERNAL "Enable audio playback devices")
set(SNOBASTE_ENCRYPT OFF CACHE INTERNAL "Base64 and encryption")
set(SNOBASTE_NETWORKING OFF CACHE INTERNAL "Networking")
set(SNOBASTE_FONT ON CACHE INTERNAL "Enable MSDF font objects")
set(SNOBASTE_REPORT OFF CACHE INTERNAL "Enable graphical reports")
set(SNOBASTE_RENDERING ON CACHE INTERNAL "Enable OpenGL rendering")
set(SNOBASTE_UI OFF CACHE INTERNAL "Enable OpenGL UI")

set(APP_ICON_RESOURCE_WINDOWS "${CMAKE_CURRENT_SOURCE_DIR}/icon.rc")
set(VERSION_RESOURCE_WINDOWS "${CMAKE_CURRENT_SOURCE_DIR}/version.rc")

find_package(glm CONFIG REQUIRED)

add_subdirectory(OneLibrary)

set(_chipiversary_cpp_headers
		Audio.hpp
		Book.hpp
		Curl.hpp
		Defines.hpp
		Ease.hpp
		Engine.hpp
		GhostWriter.hpp
		InputManager.hpp
		Loading.hpp
		Markdown.hpp
		Menu.hpp
		Renderer.hpp
		)
set(_chipiversary_cpp_sources
		Audio.cpp
		Curl.cpp
		Ease.cpp
		GhostWriter.cpp
		InputManager.cpp
		Loading.cpp
		Menu.cpp
		Renderer.cpp
		main.cpp
		)

add_executable(CHAnniversary
		WIN32
		${_chipiversary_cpp_headers}
		${_chipiversary_cpp_sources}
		${APP_ICON_RESOURCE_WINDOWS}
		${VERSION_RESOURCE_WINDOWS}
		)

target_include_directories(CHAnniversary PUBLIC 
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
		)
target_compile_features(CHAnniversary PUBLIC cxx_std_17)

find_package(glfw3 3.3 QUIET)
if(NOT TARGET glfw)
	FetchContent_Declare(glfw3
			URL https://github.com/glfw/glfw/archive/3.3.2.tar.gz
			URL_MD5 865e54ff0a100e9041a40429db98be0b
			)
	FetchContent_GetProperties(glfw3)
	if(NOT glfw3_POPULATED)
		set("GLFW_BUILD_TESTS" OFF)
		set("GLFW_BUILD_EXAMPLES" OFF)
		set("GLFW_BUILD_DOCS" OFF)
		set("GLFW_INSTALL" OFF)
		FetchContent_Populate(glfw3)
		add_subdirectory(${glfw3_SOURCE_DIR} ${glfw3_BINARY_DIR})
		set_target_properties(glfw PROPERTIES FOLDER External)
	endif()
endif()

target_link_libraries(CHAnniversary PRIVATE ${ONELIBRARY_LIBRARIES} glm glfw)