################################################################################
#
#  Copyright (c) 2021 Paul Ramsey. All rights reserved.
#  MIT License
#
################################################################################

cmake_minimum_required(VERSION 3.12)

project(GEOS VERSION 1.0.0 LANGUAGES C CXX)
find_package(GEOS 3.10 REQUIRED)

file(GLOB_RECURSE _sources ${CMAKE_CURRENT_LIST_DIR}/src/*.cpp CONFIGURE_DEPEND)
add_executable(spatial_lookup ${_sources})
target_link_libraries(spatial_lookup PRIVATE GEOS::geos)
