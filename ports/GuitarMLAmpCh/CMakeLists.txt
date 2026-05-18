cmake_minimum_required(VERSION 3.12)
project(GuitarMLAmpChSC LANGUAGES CXX)
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
set(guitarmlampch_cpp_files src/GuitarMLAmpCh.cpp)
set(guitarmlampch_sc_files Classes/GuitarMLAmpCh.sc)
set(guitarmlampch_schelp_files
    HelpSource/Classes/GuitarMLAmpCh.schelp
    HelpSource/Guides/ChowDSPPorts.schelp
)
sc_add_server_plugin("GuitarMLAmpCh" "GuitarMLAmpCh" "${guitarmlampch_cpp_files}" "${guitarmlampch_sc_files}" "${guitarmlampch_schelp_files}")
if(TARGET GuitarMLAmpCh_scsynth)
    set_target_properties(GuitarMLAmpCh_scsynth PROPERTIES OUTPUT_NAME "GuitarMLAmpCh")
endif()
install(
    FILES
        models/BluesJrAmp_VolKnob.json
        models/TS9_DriveKnob.json
        models/MesaRecMini_ModernChannel_GainKnob.json
    DESTINATION GuitarMLAmpCh/models
)
