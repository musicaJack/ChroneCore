cmake_minimum_required(VERSION 3.16)

set(PROJECT_NAME "chrone-core")
set(PROJECT_VER "0.1.0")

# idf.py / export.ps1 set IDF_PATH; CMake Tools in the IDE often do not.
if(NOT DEFINED ENV{IDF_PATH} OR "$ENV{IDF_PATH}" STREQUAL "")
    if(DEFINED ENV{USERPROFILE})
        file(GLOB _idf_candidates "$ENV{USERPROFILE}/.espressif/tools/frameworks/esp-idf-v*")
    elseif(DEFINED ENV{HOME})
        file(GLOB _idf_candidates "$ENV{HOME}/.espressif/tools/frameworks/esp-idf-v*")
    endif()
    if(_idf_candidates)
        list(SORT _idf_candidates)
        list(REVERSE _idf_candidates)
        list(GET _idf_candidates 0 _idf_path)
        set(ENV{IDF_PATH} "${_idf_path}")
        message(STATUS "IDF_PATH not set; using ${_idf_path}")
    endif()
endif()

if(NOT DEFINED ENV{IDF_PATH} OR "$ENV{IDF_PATH}" STREQUAL "")
    message(FATAL_ERROR "IDF_PATH is not set. Use an ESP-IDF terminal (idf.py build) or install ESP-IDF.")
endif()

set(_idf_project_cmake "$ENV{IDF_PATH}/tools/cmake/project.cmake")
if(NOT EXISTS "${_idf_project_cmake}")
    message(FATAL_ERROR "Invalid IDF_PATH='$ENV{IDF_PATH}': missing ${_idf_project_cmake}")
endif()

include("${_idf_project_cmake}")
project(${PROJECT_NAME})
