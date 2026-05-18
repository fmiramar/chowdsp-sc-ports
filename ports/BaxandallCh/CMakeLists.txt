cmake_minimum_required(VERSION 3.12)

project(BaxandallChSC LANGUAGES CXX)

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

set(baxandallch_cpp_files
    src/BaxandallCh.cpp
    src/dsp/BaxandallWDF.cpp
    src/dsp/BaxandallWDFAdapt.cpp
)

set(baxandallch_sc_files
    Classes/BaxandallCh.sc
)

set(baxandallch_schelp_files
    HelpSource/Classes/BaxandallCh.schelp
    HelpSource/Guides/ChowDSPPorts.schelp
)

sc_add_server_plugin(
    "BaxandallCh"
    "BaxandallCh"
    "${baxandallch_cpp_files}"
    "${baxandallch_sc_files}"
    "${baxandallch_schelp_files}"
)

if(TARGET BaxandallCh_scsynth)
    set_target_properties(BaxandallCh_scsynth PROPERTIES OUTPUT_NAME "BaxandallCh")
endif()

if(TARGET BaxandallCh_supernova)
    set_target_properties(BaxandallCh_supernova PROPERTIES OUTPUT_NAME "BaxandallCh_supernova")
endif()
