FROM  debian:12

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

# Instalace všech potřebných balíčků
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    gcc g++ cmake make git pkg-config \
    python3 python3-pip python3-venv ccache \
    vim wget mc ninja-build \
    && python3 -m pip install --break-system-packages conan \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

# Nastavení pracovního adresáře
WORKDIR /workspace

CMD ["/bin/bash"]