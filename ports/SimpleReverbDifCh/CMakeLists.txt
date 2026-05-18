cmake_minimum_required(VERSION 3.12)

project(SimpleReverbDifChSC LANGUAGES CXX)

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

set(simplereverbdifch_cpp_files
    src/SimpleReverbDifCh.cpp
)

set(simplereverbdifch_sc_files
    Classes/SimpleReverbDifCh.sc
)

set(simplereverbdifch_schelp_files
    HelpSource/Classes/SimpleReverbDifCh.schelp
    HelpSource/Guides/ChowDSPPorts.schelp
)

sc_add_server_plugin(
    "SimpleReverbDifCh"
    "SimpleReverbDifCh"
    "${simplereverbdifch_cpp_files}"
    "${simplereverbdifch_sc_files}"
    "${simplereverbdifch_schelp_files}"
)

if(TARGET SimpleReverbDifCh_scsynth)
    set_target_properties(SimpleReverbDifCh_scsynth PROPERTIES OUTPUT_NAME "SimpleReverbDifCh")
endif()

if(TARGET SimpleReverbDifCh_supernova)
    set_target_properties(SimpleReverbDifCh_supernova PROPERTIES OUTPUT_NAME "SimpleReverbDifCh_supernova")
endif()
