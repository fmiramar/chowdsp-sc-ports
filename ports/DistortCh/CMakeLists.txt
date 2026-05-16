cmake_minimum_required(VERSION 3.12)

project(DistortChSC LANGUAGES CXX)

option(SCSYNTH "Build the scsynth plugin target" ON)
option(SUPERNOVA "Build the supernova plugin target" OFF)
option(STRICT "Enable strict compiler warnings" OFF)
option(NATIVE "Enable -march=native where supported" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT SC_PATH)
    set(SC_PATH "../../supercollider")
endif()

get_filename_component(SC_PATH_FOR_MODULES "${SC_PATH}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

list(APPEND CMAKE_MODULE_PATH "${SC_PATH_FOR_MODULES}/tools/cmake_gen")

include(SuperColliderServerPlugin)
sc_do_initial_compiler_config()
sc_check_sc_path("${SC_PATH}")

set(distortch_cpp_files
    src/DistortCh.cpp
)

set(distortch_sc_files
    Classes/DistortCh.sc
)

set(distortch_schelp_files
    HelpSource/Classes/DistortCh.schelp
)

sc_add_server_plugin(
    "DistortCh"
    "DistortCh"
    "${distortch_cpp_files}"
    "${distortch_sc_files}"
    "${distortch_schelp_files}"
)

if(TARGET DistortCh_scsynth)
    set_target_properties(DistortCh_scsynth PROPERTIES OUTPUT_NAME "DistortCh")
endif()

if(TARGET DistortCh_supernova)
    set_target_properties(DistortCh_supernova PROPERTIES OUTPUT_NAME "DistortCh_supernova")
endif()
