cmake_minimum_required(VERSION 3.20)
project(CompiledLanguage VERSION 1.0 LANGUAGES CXX C)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23)


find_package(LLVM REQUIRED CONFIG)

add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})
llvm_map_components_to_libnames(llvm_libs core support)

# Dynamically find all .cpp files in the project directory
file(GLOB_RECURSE SRC_FILES *.cpp)

# Filter out anything inside the .cache folder if it exists
list(FILTER SRC_FILES EXCLUDE REGEX "/\\.cache/")
list(FILTER SRC_FILES EXCLUDE REGEX "/\\build/")

# Create the executable using all the discovered C++ files
add_executable(my_program ${SRC_FILES})

target_link_libraries(my_program PRIVATE ${llvm_libs})
