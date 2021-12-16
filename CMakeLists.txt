################################################################################
#
#  Copyright (c) 2021 Paul Ramsey. All rights reserved.
#  MIT License
#
################################################################################

cmake_minimum_required(VERSION 3.12)

project(GEOS VERSION 1.0.0 LANGUAGES C CXX)
find_package(GEOS 3.10 REQUIRED)

add_executable(spatial_lookup SpatialLookup.cpp)
target_link_libraries(spatial_lookup PRIVATE GEOS::geos)
