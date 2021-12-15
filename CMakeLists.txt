################################################################################
# Part of CMake configuration for GEOS
#
# Copyright (C) 2018 Mateusz Loskot <mateusz@loskot.net>
#
# This is free software; you can redistribute and/or modify it under
# the terms of the GNU Lesser General Public Licence as published
# by the Free Software Foundation.
# See the COPYING file for more information.
################################################################################
cmake_minimum_required(VERSION 3.12)

project(GEOS VERSION 1.0.0 LANGUAGES C CXX)
find_package(GEOS 3.10 REQUIRED)

add_executable(spatial_lookup SpatialLookup.cpp)
target_link_libraries(spatial_lookup PRIVATE GEOS::geos)
