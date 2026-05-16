cmake_minimum_required(VERSION 3.12)

project(WaveMultiplierChSC LANGUAGES CXX)

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

set(wavemultiplierch_cpp_files
    src/WaveMultiplierCh.cpp
)

set(wavemultiplierch_sc_files
    Classes/WaveMultiplierCh.sc
)

set(wavemultiplierch_schelp_files
    HelpSource/Classes/WaveMultiplierCh.schelp
)

sc_add_server_plugin(
    "WaveMultiplierCh"
    "WaveMultiplierCh"
    "${wavemultiplierch_cpp_files}"
    "${wavemultiplierch_sc_files}"
    "${wavemultiplierch_schelp_files}"
)

if(TARGET WaveMultiplierCh_scsynth)
    set_target_properties(WaveMultiplierCh_scsynth PROPERTIES OUTPUT_NAME "WaveMultiplierCh")
endif()

if(TARGET WaveMultiplierCh_supernova)
    set_target_properties(WaveMultiplierCh_supernova PROPERTIES OUTPUT_NAME "WaveMultiplierCh_supernova")
endif()
