#FROM --platform=linux/arm64 debian:12
FROM --platform=linux/amd64 debian:12

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
    python3 python3-pip ccache \
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

# Přidání ~/.local/bin do PATH pro uživatele
ENV PATH="/home/${USERNAME}/.local/bin:${PATH}"

# Instalace Conan jako uživatel
RUN pip3 install --break-system-packages conan && \
    echo "Conan installed successfully"

# Přepneme zpět na root pro entrypoint
USER root

# Nastavení pracovního adresáře
WORKDIR /workspace

ENTRYPOINT ["/entrypoint.sh"]
CMD ["/bin/bash"]