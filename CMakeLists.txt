cmake_minimum_required(VERSION 3.20)

set(PROJECT_NAME "DX11Rendering2DDemo")

project(${PROJECT_NAME} VERSION 1.0 LANGUAGES CXX)

aux_source_directory(./src SOURCE_FILES)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 20)
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /Zc:__cplusplus /utf-8 /MP)
endif()
if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    target_compile_definitions(${PROJECT_NAME} PRIVATE -DDEBUG)
endif()
target_compile_definitions(${PROJECT_NAME} PRIVATE -DCMAKE_PROJECT_NAME="${PROJECT_NAME}")
target_link_libraries(${PROJECT_NAME} D3D11.lib DXGI.lib d3dcompiler.lib)
