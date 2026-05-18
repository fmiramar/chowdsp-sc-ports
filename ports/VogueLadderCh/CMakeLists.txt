cmake_minimum_required(VERSION 3.12)

project(VogueLadderChSC LANGUAGES CXX)

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

set(vogueladderch_cpp_files
    src/VogueLadderCh.cpp
    src/dsp/HighPassOnePole.cpp
    src/dsp/LowPassOnePole.cpp
    src/dsp/HighPassLadder.cpp
    src/dsp/LowPassLadder.cpp
)

set(vogueladderch_sc_files
    Classes/VogueLadderCh.sc
)

set(vogueladderch_schelp_files
    HelpSource/Classes/VogueLadderCh.schelp
    HelpSource/Guides/ChowDSPPorts.schelp
)

sc_add_server_plugin(
    "VogueLadderCh"
    "VogueLadderCh"
    "${vogueladderch_cpp_files}"
    "${vogueladderch_sc_files}"
    "${vogueladderch_schelp_files}"
)

if(TARGET VogueLadderCh_scsynth)
    set_target_properties(VogueLadderCh_scsynth PROPERTIES OUTPUT_NAME "VogueLadderCh")
endif()

if(TARGET VogueLadderCh_supernova)
    set_target_properties(VogueLadderCh_supernova PROPERTIES OUTPUT_NAME "VogueLadderCh_supernova")
endif()
