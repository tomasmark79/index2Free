#!/bin/bash

# Cleanup script pro změnu oprávnění souborů vytvořených root uživatelem v kontejneru
# na hostitelského uživatele

USER_ID=$(id -u)
GROUP_ID=$(id -g)

echo "Changing ownership of files created by Docker container..."
echo "Setting ownership to ${USER_ID}:${GROUP_ID}"

# Změníme oprávnění pro build directory
if [ -d "build" ]; then
    echo "Fixing ownership for build/ directory..."
    sudo chown -R ${USER_ID}:${GROUP_ID} build/
fi

# Změníme oprávnění pro .conan2 directory pokud existuje
if [ -d ".conan2" ]; then
    echo "Fixing ownership for .conan2/ directory..."
    sudo chown -R ${USER_ID}:${GROUP_ID} .conan2/
fi

# Změníme oprávnění pro všechny soubory vytvořené rootem
echo "Fixing ownership for all root-owned files in current directory..."
sudo find . -user root -exec chown ${USER_ID}:${GROUP_ID} {} \; 2>/dev/null || true

echo "Cleanup completed!"
