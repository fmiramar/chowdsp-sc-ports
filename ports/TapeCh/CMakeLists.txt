cmake_minimum_required(VERSION 3.12)

project(TapeChSC LANGUAGES CXX)

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

set(tapech_cpp_files
    src/TapeCh.cpp
)

set(tapech_sc_files
    Classes/TapeCh.sc
)

set(tapech_schelp_files
    HelpSource/Classes/TapeCh.schelp
    HelpSource/Guides/ChowDSPPorts.schelp
)

sc_add_server_plugin(
    "TapeCh"
    "TapeCh"
    "${tapech_cpp_files}"
    "${tapech_sc_files}"
    "${tapech_schelp_files}"
)

if(TARGET TapeCh_scsynth)
    set_target_properties(TapeCh_scsynth PROPERTIES OUTPUT_NAME "TapeCh")
endif()

if(TARGET TapeCh_supernova)
    set_target_properties(TapeCh_supernova PROPERTIES OUTPUT_NAME "TapeCh_supernova")
endif()
