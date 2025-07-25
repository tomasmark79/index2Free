#!/bin/bash
set -e

# Vytvoříme složku pokud neexistuje a nastavíme oprávnění s force
mkdir -p /home/${USERNAME}/.conan2 || true
chmod 755 /home/${USERNAME}/.conan2 || true
chown -R ${USERNAME}:${USERNAME} /home/${USERNAME}/.conan2 2>/dev/null || true

# Zkontrolujeme a případně vytvoříme Conan profil pokud neexistuje
if [ ! -f "/home/${USERNAME}/.conan2/profiles/default" ]; then
    # Přepneme na uživatele pro vytvoření profilu
    gosu ${USERNAME} conan profile detect || true
fi

# Přidáme konfiguraci pokud neexistuje (nezávisle na tom, jestli profil existoval)
if [ -f "/home/${USERNAME}/.conan2/profiles/default" ] && ! grep -q "tools.system.package_manager:sudo=true" "/home/${USERNAME}/.conan2/profiles/default" 2>/dev/null; then
    gosu ${USERNAME} bash -c "
        echo '' >> /home/${USERNAME}/.conan2/profiles/default
        echo '[conf]' >> /home/${USERNAME}/.conan2/profiles/default
        echo 'tools.system.package_manager:sudo=true' >> /home/${USERNAME}/.conan2/profiles/default
        echo 'tools.system.package_manager:mode=install' >> /home/${USERNAME}/.conan2/profiles/default
    "
fi

# Přidáme do .bashrc příkaz pro přepnutí do /workspace
echo "cd /workspace" >> /home/${USERNAME}/.bashrc

# Přepneme na neprivilegovaného uživatele a spustíme původní příkaz
exec gosu ${USERNAME} "$@"
