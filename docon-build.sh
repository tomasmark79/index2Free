#!/bin/bash

set -e  # Exit immediately if a command exits with a non-zero status

echo "=== Starting Docker Dev Build Process ==="

# Step 1: Export Conan package
echo "Step 1: Exporting Conan package..."
if conan export ~/.conan2/tomaspack/m4/ --name=m4 --version=1.4.20 --user=local --channel=stable; then
    echo "✓ Conan export successful"
else
    echo "✗ Conan export failed"
    exit 1
fi

# Step 2: Install Conan dependencies
echo "Step 2: Installing Conan dependencies..."
if conan install . --output-folder="./build/standalone/dockerhell/debug" --deployer=full_deploy --build=missing --settings build_type=Debug; then
    echo "✓ Conan install successful"
else
    echo "✗ Conan install failed"
    exit 1
fi

# Step 3: Configure CMake
echo "Step 3: Configuring CMake..."
if source "./build/standalone/dockerhell/debug/conanbuild.sh" && cmake -S "./standalone" -B "./build/standalone/dockerhell/debug" -DCMAKE_TOOLCHAIN_FILE="/workspace/build/standalone/dockerhell/debug/conan_toolchain.cmake" -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX="/workspace/build/installation/dockerhell/debug"; then
    echo "✓ CMake configuration successful"
else
    echo "✗ CMake configuration failed"
    exit 1
fi

# Step 4: Build project
echo "Step 4: Building project..."
if source "./build/standalone/dockerhell/debug/conanbuild.sh" && cmake --build "./build/standalone/dockerhell/debug" -j $(nproc); then
    echo "✓ Build successful"
else
    echo "✗ Build failed"
    exit 1
fi

echo "=== All steps completed successfully! ==="