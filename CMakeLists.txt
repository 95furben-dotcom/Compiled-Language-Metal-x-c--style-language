cmake_minimum_required(VERSION 3.20)
project(CompiledLanguage VERSION 1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23)

# --- DYNAMIC LLVM DETECTION (FULLY ROBUST) ---
# Check if we are on Apple macOS and find the Homebrew installation prefix dynamically
if(APPLE)
    # Dynamically find the brew binary regardless of architecture (Intel or Apple Silicon)
    find_program(BREW_EXECUTABLE NAMES brew PATHS "/opt/homebrew/bin" "/usr/local/bin")
    
    if(BREW_EXECUTABLE)
        # Query homebrew directly for where it placed LLVM
        execute_process(
            COMMAND ${BREW_EXECUTABLE} --prefix llvm
            OUTPUT_VARIABLE HOMEBREW_LLVM_PREFIX
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        # Inject the discovered prefix and its cmake subdirectory straight into CMake paths
        if(HOMEBREW_LLVM_PREFIX)
            list(APPEND CMAKE_PREFIX_PATH "${HOMEBREW_LLVM_PREFIX}")
            list(APPEND CMAKE_PREFIX_PATH "${HOMEBREW_LLVM_PREFIX}/lib/cmake/llvm")
        endif()
    endif()
endif()

find_package(LLVM REQUIRED CONFIG)
include(LLVM-Config)
add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})

# Dynamically find all .cpp files in the project directory
file(GLOB_RECURSE SRC_FILES *.cpp)

# Filter out anything inside the .cache folder if it exists
list(FILTER SRC_FILES EXCLUDE REGEX "/\\.cache/")
list(FILTER SRC_FILES EXCLUDE REGEX "/\\build/")
list(FILTER SRC_FILES EXCLUDE REGEX "/\\LLVMParser/FunctionEmiter/")

add_executable(my_program ${SRC_FILES})

target_include_directories(my_program PRIVATE ${LLVM_INCLUDE_DIRS})
target_compile_definitions(my_program PRIVATE ${LLVM_DEFINITIONS_LIST})

# Pick the components you actually use (add more as needed)
llvm_map_components_to_libnames(llvm_libs
    core support irreader passes analysis
    native    # target for the host (arm64)
)
target_link_libraries(my_program PRIVATE ${llvm_libs})