cmake_minimum_required(VERSION 3.12)

project(TapeDegradeChSC LANGUAGES CXX)

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

set(tapedegradech_cpp_files
    src/TapeDegradeCh.cpp
)

set(tapedegradech_sc_files
    Classes/TapeDegradeCh.sc
)

set(tapedegradech_schelp_files
    HelpSource/Classes/TapeDegradeCh.schelp
)

sc_add_server_plugin(
    "TapeDegradeCh"
    "TapeDegradeCh"
    "${tapedegradech_cpp_files}"
    "${tapedegradech_sc_files}"
    "${tapedegradech_schelp_files}"
)

if(TARGET TapeDegradeCh_scsynth)
    set_target_properties(TapeDegradeCh_scsynth PROPERTIES OUTPUT_NAME "TapeDegradeCh")
endif()

if(TARGET TapeDegradeCh_supernova)
    set_target_properties(TapeDegradeCh_supernova PROPERTIES OUTPUT_NAME "TapeDegradeCh_supernova")
endif()
