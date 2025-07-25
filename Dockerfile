FROM  debian:12

# Argumenty pro uživatele (předané z docker-compose nebo build)
ARG USER_ID=1000
ARG GROUP_ID=1000
ARG USERNAME=developer

# Sloučené ENV proměnné pro lepší layering
ENV DEBIAN_FRONTEND=noninteractive \
    CC=gcc \
    CXX=g++ \
    AR=ar \
    STRIP=strip \
    CMAKE_C_COMPILER_LAUNCHER=ccache \
    CMAKE_CXX_COMPILER_LAUNCHER=ccache \
    CCACHE_DIR=/workspace/.ccache \
    QEMU_CPU=max \
    QEMU_STRACE=0 \
    QEMU_LOG_FILENAME=/dev/null \
    MAKEFLAGS=-j$(nproc)

# Kombinovaná instalace bez Conan + vytvoření uživatele
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    gcc g++ cmake make git pkg-config \
    python3 python3-pip python3-venv ccache \
    vim wget mc ninja-build sudo gosu \
    && groupadd -g ${GROUP_ID} ${USERNAME} \
    && useradd -u ${USER_ID} -g ${GROUP_ID} -m -s /bin/bash ${USERNAME} \
    && echo "${USERNAME} ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

# Zkopírujeme entrypoint skript
COPY entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

USER ${USERNAME}

# Vytvoření virtuálního prostředí a instalace Conan
RUN python3 -m venv /home/${USERNAME}/.conan-venv && \
    /home/${USERNAME}/.conan-venv/bin/pip install --upgrade pip setuptools wheel && \
    /home/${USERNAME}/.conan-venv/bin/pip install conan && \
    echo "Conan installed successfully" && \
    /home/${USERNAME}/.conan-venv/bin/conan --version

# Přidání conan do PATH
ENV PATH="/home/${USERNAME}/.conan-venv/bin:${PATH}"

# Přepneme zpět na root pro entrypoint
USER root

# Nastavení pracovního adresáře
WORKDIR /workspace

ENTRYPOINT ["/entrypoint.sh"]
CMD ["/bin/bash"]