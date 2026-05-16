cmake_minimum_required(VERSION 3.12)

project(TR808HatChSC LANGUAGES CXX)

option(SCSYNTH "Build the scsynth plugin target" ON)
option(SUPERNOVA "Build the supernova plugin target" OFF)
option(STRICT "Enable strict compiler warnings" OFF)
option(NATIVE "Enable -march=native where supported" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

if(NOT SC_PATH)
    set(SC_PATH "../../supercollider")
endif()

get_filename_component(SC_PATH_FOR_MODULES "${SC_PATH}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

list(APPEND CMAKE_MODULE_PATH "${SC_PATH_FOR_MODULES}/tools/cmake_gen")

include(SuperColliderServerPlugin)
sc_do_initial_compiler_config()
sc_check_sc_path("${SC_PATH}")

set(tr808hatch_cpp_files
    src/TR808HatCh.cpp
)

set(tr808hatch_sc_files
    Classes/TR808HatCh.sc
)

set(tr808hatch_schelp_files
    HelpSource/Classes/TR808HatCh.schelp
)

sc_add_server_plugin(
    "TR808HatCh"
    "TR808HatCh"
    "${tr808hatch_cpp_files}"
    "${tr808hatch_sc_files}"
    "${tr808hatch_schelp_files}"
)

if(TARGET TR808HatCh_scsynth)
    set_target_properties(TR808HatCh_scsynth PROPERTIES OUTPUT_NAME "TR808HatCh")
endif()

if(TARGET TR808HatCh_supernova)
    set_target_properties(TR808HatCh_supernova PROPERTIES OUTPUT_NAME "TR808HatCh_supernova")
endif()
