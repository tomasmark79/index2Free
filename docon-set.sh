#!/bin/bash

#  Set up Conan environment for Docker development
# 
# LTO (Link Time Optimization) can cause issues with some libraries, so we disable it in the Conan profile.

CONAN_PROFILE_DIR="/root/.conan2/profiles"
CONAN_PROFILE_FILE="$CONAN_PROFILE_DIR/default"

echo "Configuring Conan profile..."

# Vytvoříme Conan profil pokud neexistuje
if [ ! -f "$CONAN_PROFILE_FILE" ]; then
    echo "Creating Conan profile..."
    mkdir -p "$CONAN_PROFILE_DIR"
    conan profile detect
fi

# Přidáme konfiguraci pokud neexistuje
if [ -f "$CONAN_PROFILE_FILE" ] && ! grep -q "tools.system.package_manager:sudo=true" "$CONAN_PROFILE_FILE" 2>/dev/null; then
    echo "Adding system package manager configuration to Conan profile..."
    
    # Zkontrolujeme, jestli už existuje [conf] sekce
    if ! grep -q "^\[conf\]" "$CONAN_PROFILE_FILE"; then
        echo "" >> "$CONAN_PROFILE_FILE"
        echo "[conf]" >> "$CONAN_PROFILE_FILE"
    fi

    # Přidáme nastavení
    echo "tools.system.package_manager:mode=install" >> "$CONAN_PROFILE_FILE"
    echo "tools.system.package_manager:sudo=true" >> "$CONAN_PROFILE_FILE"
    echo "tools.build:cxxflags=[\"-fno-lto\"]" >> "$CONAN_PROFILE_FILE"
    echo "tools.build:cflags=[\"-fno-lto\"]" >> "$CONAN_PROFILE_FILE"
    echo "tools.build:shared_linker_flags=[\"-fno-lto\"]" >> "$CONAN_PROFILE_FILE"

    echo "Conan profile configured successfully!"
else
    echo "Conan profile already configured."
fi

echo "Current Conan profile:"
echo "===================="
cat "$CONAN_PROFILE_FILE"
