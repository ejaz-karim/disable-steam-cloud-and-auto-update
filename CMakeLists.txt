cmake_minimum_required(VERSION 3.0.0)
project(disable_steam_cloud_and_auto_update VERSION 0.1.0 LANGUAGES C CXX)

include(CTest)
enable_testing()

add_executable(disable_steam_cloud_and_auto_update main.cpp)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
