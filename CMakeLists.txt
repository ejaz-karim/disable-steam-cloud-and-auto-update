cmake_minimum_required(VERSION 3.5)
project(disable-steam-cloud-and-auto-update VERSION 0.1.0 LANGUAGES C CXX)

include(CTest)
enable_testing()

link_directories(
  C:/vcpkg/installed/x64-windows/lib/
)

include_directories(
  include
  C:/vcpkg/installed/x64-windows/include/

)
set(SOURCES 
  src/main.cpp
  src/api.cpp 
  src/autoupdate_disable.cpp
  src/cloud_disable.cpp
  src/utility.cpp
)

add_executable(disable-steam-cloud-and-auto-update ${SOURCES})

TARGET_LINK_LIBRARIES(disable-steam-cloud-and-auto-update
  drogon
)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
