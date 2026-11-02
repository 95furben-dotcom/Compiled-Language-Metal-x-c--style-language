#!/bin/bash
set -e

PROJECT_ROOT=$1

mkdir -p .cache

# Wiping build cache forces CMake to evaluate fresh flags
rm -rf build 

# Compile app
echo "Compiling app..."
# Configure (generates the build files and compile_commands.json)
cmake -B build \
    -DLLVM_DIR="$(brew --prefix llvm)/lib/cmake/llvm" \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -DCMAKE_CXX_FLAGS="-DLLVM_DISABLE_ABI_BREAKING_CHECKS=1"

# Compile the project 
cmake --build build


# Run
echo "Running..."
echo 
echo 
./"build/my_program"

echo 
echo