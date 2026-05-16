cmake_minimum_required(VERSION 3.12)

project(ModalSpringChSC LANGUAGES CXX)

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

set(modalspringch_cpp_files
    src/ModalSpringCh.cpp
)

set(modalspringch_sc_files
    Classes/ModalSpringCh.sc
)

set(modalspringch_schelp_files
    HelpSource/Classes/ModalSpringCh.schelp
)

sc_add_server_plugin(
    "ModalSpringCh"
    "ModalSpringCh"
    "${modalspringch_cpp_files}"
    "${modalspringch_sc_files}"
    "${modalspringch_schelp_files}"
)

if(TARGET ModalSpringCh_scsynth)
    set_target_properties(ModalSpringCh_scsynth PROPERTIES OUTPUT_NAME "ModalSpringCh")
endif()

if(TARGET ModalSpringCh_supernova)
    set_target_properties(ModalSpringCh_supernova PROPERTIES OUTPUT_NAME "ModalSpringCh_supernova")
endif()
