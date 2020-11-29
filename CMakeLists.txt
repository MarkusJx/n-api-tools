# CMakeLists.txt for use with cmake-js
# It would be used to build this project, if it was not designed to delete the whole project, because this seems like a great idea
cmake_minimum_required(VERSION 3.12)
project(napi_tools CXX)

set(CMAKE_CXX_STANDARD 20)

# Get the include dir
execute_process(COMMAND node -p "require('.').include"
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                OUTPUT_VARIABLE N_API_TOOLS_DIR)

include_directories(${CMAKE_JS_INC} ${N_API_TOOLS_DIR})
add_compile_definitions(NAPI_CPP_EXCEPTIONS)

set(SRC main.cpp)

add_library(${PROJECT_NAME} SHARED ${SRC} ${CMAKE_JS_SRC})

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "" SUFFIX ".node")
target_link_libraries(${PROJECT_NAME} ${CMAKE_JS_LIB})

# Include N-API wrappers
execute_process(COMMAND node -p "require('node-addon-api').include"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE NODE_ADDON_API_DIR
        )
string(REPLACE "\n" "" NODE_ADDON_API_DIR ${NODE_ADDON_API_DIR})
string(REPLACE "\"" "" NODE_ADDON_API_DIR ${NODE_ADDON_API_DIR})
target_include_directories(${PROJECT_NAME} PRIVATE ${NODE_ADDON_API_DIR})

# define NPI_VERSION
add_definitions(-DNAPI_VERSION=4)
