cmake_minimum_required(VERSION 3.12)

project(PolygonalOscChSC LANGUAGES CXX)

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

set(polygonaloscch_cpp_files
    src/PolygonalOscCh.cpp
)

set(polygonaloscch_sc_files
    Classes/PolygonalOscCh.sc
)

set(polygonaloscch_schelp_files
    HelpSource/Classes/PolygonalOscCh.schelp
)

sc_add_server_plugin(
    "PolygonalOscCh"
    "PolygonalOscCh"
    "${polygonaloscch_cpp_files}"
    "${polygonaloscch_sc_files}"
    "${polygonaloscch_schelp_files}"
)

if(TARGET PolygonalOscCh_scsynth)
    set_target_properties(PolygonalOscCh_scsynth PROPERTIES OUTPUT_NAME "PolygonalOscCh")
endif()

if(TARGET PolygonalOscCh_supernova)
    set_target_properties(PolygonalOscCh_supernova PROPERTIES OUTPUT_NAME "PolygonalOscCh_supernova")
endif()
