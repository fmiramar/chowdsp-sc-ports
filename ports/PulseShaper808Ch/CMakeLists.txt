cmake_minimum_required(VERSION 3.12)

project(PulseShaper808ChSC LANGUAGES CXX)

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

set(pulseshaper808ch_cpp_files
    src/PulseShaper808Ch.cpp
)

set(pulseshaper808ch_sc_files
    Classes/PulseShaper808Ch.sc
)

set(pulseshaper808ch_schelp_files
    HelpSource/Classes/PulseShaper808Ch.schelp
)

sc_add_server_plugin(
    "PulseShaper808Ch"
    "PulseShaper808Ch"
    "${pulseshaper808ch_cpp_files}"
    "${pulseshaper808ch_sc_files}"
    "${pulseshaper808ch_schelp_files}"
)

if(TARGET PulseShaper808Ch_scsynth)
    set_target_properties(PulseShaper808Ch_scsynth PROPERTIES OUTPUT_NAME "PulseShaper808Ch")
endif()

if(TARGET PulseShaper808Ch_supernova)
    set_target_properties(PulseShaper808Ch_supernova PROPERTIES OUTPUT_NAME "PulseShaper808Ch_supernova")
endif()
