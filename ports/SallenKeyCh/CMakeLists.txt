cmake_minimum_required(VERSION 3.12)

project(SallenKeyChSC LANGUAGES CXX)

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

set(sallenkeych_cpp_files
    src/SallenKeyCh.cpp
    src/dsp/SallenKeyLPF.cpp
)

set(sallenkeych_sc_files
    Classes/SallenKeyCh.sc
)

set(sallenkeych_schelp_files
    HelpSource/Classes/SallenKeyCh.schelp
)

sc_add_server_plugin(
    "SallenKeyCh"
    "SallenKeyCh"
    "${sallenkeych_cpp_files}"
    "${sallenkeych_sc_files}"
    "${sallenkeych_schelp_files}"
)

if(TARGET SallenKeyCh_scsynth)
    set_target_properties(SallenKeyCh_scsynth PROPERTIES OUTPUT_NAME "SallenKeyCh")
endif()

if(TARGET SallenKeyCh_supernova)
    set_target_properties(SallenKeyCh_supernova PROPERTIES OUTPUT_NAME "SallenKeyCh_supernova")
endif()
