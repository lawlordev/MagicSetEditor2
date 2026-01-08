cmake_minimum_required(VERSION 3.13)

project(mse3 VERSION 3.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(wxWidgets 3 REQUIRED COMPONENTS core base net html)
find_package(Boost REQUIRED COMPONENTS regex)

find_package(PkgConfig)

# nlohmann/json for GitHub API parsing
include(FetchContent)
FetchContent_Declare(json
  URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)
FetchContent_MakeAvailable(json)

# find hunspell
pkg_check_modules(HUNSPELL hunspell)
if( NOT HUNSPELL_FOUND )
	message("-- Cannot find Hunspell via pkg-config, checking directly...")
	find_path(HUNSPELL_INCLUDE_DIRS hunspell/hunspell.hxx)
	find_library(HUNSPELL_LIBRARIES NAMES hunspell libhunspell)
	if ( ${HUNSPELL_INCLUDE_DIRS} STREQUAL "HUNSPELL_INCLUDE_DIRS-NOTFOUND" )
		message(FATAL_ERROR "Hunspell cannot be found")
	else()
		message("-- Found Hunspell at ${HUNSPELL_LIBRARIES}")
	endif()
endif()

include_directories("${PROJECT_BINARY_DIR}/src")
include_directories("${PROJECT_SOURCE_DIR}/src")
include(${wxWidgets_USE_FILE})
include_directories(${Boost_INCLUDE_DIRS})
include_directories(${HUNSPELL_INCLUDE_DIRS})

# MSE3 executable

add_executable(mse3 WIN32)
target_link_libraries(${PROJECT_NAME} ${wxWidgets_LIBRARIES})
target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES})
target_link_libraries(${PROJECT_NAME} ${HUNSPELL_LIBRARIES})
target_link_libraries(${PROJECT_NAME} nlohmann_json::nlohmann_json)

file(GLOB_RECURSE sources src/*.cpp)
list(FILTER sources EXCLUDE REGEX win32_cli_wrapper.cpp)
target_sources(mse3 PRIVATE ${sources})
target_precompile_headers(mse3 PRIVATE src/util/prec.hpp)

configure_file(src/config.hpp.in src/config.hpp)

# resource file

target_sources(mse3 PRIVATE resource/win32_res.rc)

# mse3.com: wrapper to enable command line executable on windows

if(WIN32)
  add_executable(mse3-cli)
  target_sources(mse3-cli PRIVATE src/cli/win32_cli_wrapper.cpp)
  add_custom_command(TARGET mse3-cli POST_BUILD COMMAND
    ${CMAKE_COMMAND} -E copy "${PROJECT_BINARY_DIR}/mse3-cli.exe" "${PROJECT_BINARY_DIR}/mse3.com"
  )
endif()

# macOS App Bundle
if(APPLE)
  set_target_properties(mse3 PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.mse3.app"
    MACOSX_BUNDLE_BUNDLE_NAME "MSE3"
    MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/tools/macos/Info.plist.in"
    MACOSX_BUNDLE_ICON_FILE "mse3.icns"
  )

  # Copy icon into bundle
  set(MACOS_ICON "${CMAKE_SOURCE_DIR}/tools/macos/mse3.icns")
  set_source_files_properties(${MACOS_ICON} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
  target_sources(mse3 PRIVATE ${MACOS_ICON})

  # Copy resource directory into bundle after build
  add_custom_command(TARGET mse3 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
      "${CMAKE_SOURCE_DIR}/resource"
      "$<TARGET_BUNDLE_DIR:mse3>/Contents/SharedSupport/resource"
    COMMENT "Copying resources into app bundle"
  )
endif()

# warnings
if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
  # Update if necessary
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wsuggest-override -Wstrict-null-sentinel -Wno-comment -Wno-unused-parameter")
endif()

# visual studio debugger
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT mse3)

# Static linking on windows
if (${VCPKG_TARGET_TRIPLET} MATCHES ".*-static")
  message("Static linking")
  set(CompilerFlags CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE CMAKE_CXX_FLAGS_RELWITHDEBINFO CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE)
  foreach(CompilerFlag ${CompilerFlags})
    string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
  endforeach()
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /GL")
  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /GL")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /LTCG")
  # Need explicit dependencies on wx's dependencies
  find_package(png REQUIRED)
  find_package(tiff REQUIRED)
  find_package(jpeg REQUIRED)
  find_package(zlib REQUIRED)
  target_link_libraries(${PROJECT_NAME} ${PNG_LIBRARIES})
  target_link_libraries(${PROJECT_NAME} ${JPEG_LIBRARIES})
  target_link_libraries(${PROJECT_NAME} ${ZLIB_LIBRARIES})
  target_link_libraries(${PROJECT_NAME} ${TIFF_LIBRARIES})
  # Defines
  add_definitions(-DSTATIC)
  add_definitions(-DHUNSPELL_STATIC)
endif()

# Test suite
include(test/tests.cmake)
