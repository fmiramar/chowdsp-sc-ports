cmake_minimum_required(VERSION 3.12)

project(BassFaceChSC LANGUAGES CXX)

option(SCSYNTH "Build the scsynth plugin target" ON)
option(SUPERNOVA "Build the supernova plugin target" OFF)

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

set(bassfacech_cpp_files src/BassFaceCh.cpp)
set(bassfacech_sc_files Classes/BassFaceCh.sc)
set(bassfacech_schelp_files HelpSource/Classes/BassFaceCh.schelp)

sc_add_server_plugin("BassFaceCh" "BassFaceCh" "${bassfacech_cpp_files}" "${bassfacech_sc_files}" "${bassfacech_schelp_files}")

if(TARGET BassFaceCh_scsynth)
    set_target_properties(BassFaceCh_scsynth PROPERTIES OUTPUT_NAME "BassFaceCh")
endif()

install(
    FILES
        ../../BYOD/res/guitar_ml_models/bass_face_model_96k.json
        ../../BYOD/res/guitar_ml_models/bass_face_model_88_2k.json
    DESTINATION BassFaceCh/models
)
