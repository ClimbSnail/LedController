# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

set(LEDC_SW_VER "1.3.2")

add_definitions(
    -DLEDC_HW_VERSION=0
    -DLEDC_SW_VERSION="${LEDC_SW_VER}"
    )

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -g -Wno-format-truncation")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -g -Wno-format-truncation")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -g -Wno-stringop-truncation")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -g -Wno-stringop-truncation")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -g -Wno-array-bounds")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -g -Wno-array-bounds")

# idf_build_set_property(COMPILE_OPTIONS "-Werror=format-truncation" APPEND)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(LEDC_SW_v${LEDC_SW_VER})
