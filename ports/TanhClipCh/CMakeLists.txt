cmake_minimum_required(VERSION 3.12)

project(TanhClipChSC LANGUAGES CXX)

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

set(tanhclipch_cpp_files
    src/TanhClipCh.cpp
)

set(tanhclipch_sc_files
    Classes/TanhClipCh.sc
)

set(tanhclipch_schelp_files
    HelpSource/Classes/TanhClipCh.schelp
    HelpSource/Guides/ChowDSPPorts.schelp
)

sc_add_server_plugin(
    "TanhClipCh"
    "TanhClipCh"
    "${tanhclipch_cpp_files}"
    "${tanhclipch_sc_files}"
    "${tanhclipch_schelp_files}"
)

if(TARGET TanhClipCh_scsynth)
    set_target_properties(TanhClipCh_scsynth PROPERTIES OUTPUT_NAME "TanhClipCh")
endif()

if(TARGET TanhClipCh_supernova)
    set_target_properties(TanhClipCh_supernova PROPERTIES OUTPUT_NAME "TanhClipCh_supernova")
endif()
