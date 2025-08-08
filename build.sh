#!/usr/bin/env bash

set -euo pipefail

# ----------------------------------------
# Preflight: tool checks and environment
# ----------------------------------------
die() { echo "[build] ERROR: $*" >&2; exit 1; }

check_cmd() { command -v "$1" >/dev/null 2>&1 || die "Required tool '$1' is not in PATH."; }

check_cmd conan
check_cmd cmake

# Optional: show versions for easier troubleshooting
echo "[build] conan: $(conan --version || echo unknown)"
echo "[build] cmake: $(cmake --version | head -n1 || echo unknown)"

# Detect platform (linux/macos/windows via MSYS/MINGW/CYGWIN or Windows_NT)
UNAME_S="${OS:-}"
if [[ -z "$UNAME_S" ]]; then
	UNAME_S="$(uname -s 2>/dev/null || echo unknown)"
fi
case "$UNAME_S" in
	*Windows_NT*|MSYS*|MINGW*|CYGWIN*) HOST_OS="windows";;
	Linux*) HOST_OS="linux";;
	Darwin*) HOST_OS="mac";;
	*) HOST_OS="unknown";;
esac
echo "[build] host OS detected: $HOST_OS ($UNAME_S)"

# ----------------------------------------
# Layout
# ----------------------------------------
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$ROOT_DIR/build/standalone/default/debug"
INSTALL_DIR="$ROOT_DIR/build/installation/default/debug"

mkdir -p "$BUILD_DIR" "$INSTALL_DIR"

# Ensure conan default profile exists
if ! conan profile list 2>/dev/null | grep -q "^default$"; then
	echo "[build] conan profile 'default' not found. Running 'conan profile detect --force'..."
	conan profile detect --force || die "Failed to detect/create conan default profile"
fi

# ----------------------------------------
# Conan install
# ----------------------------------------
echo "[build] Running Conan install..."
conan install "$ROOT_DIR" \
	--output-folder="$BUILD_DIR" \
	--deployer=full_deploy \
	--build=missing \
	--profile default \
	--settings build_type=Debug

# ----------------------------------------
# Helper: run commands inside Conan build env
# ----------------------------------------
run_in_conan_env() {
	# Usage: run_in_conan_env <command ...>
	if [[ -f "$BUILD_DIR/conanbuild.sh" ]]; then
		# shellcheck disable=SC1090
		source "$BUILD_DIR/conanbuild.sh"
		"$@"
	elif [[ -f "$BUILD_DIR/conanbuild.bat" ]]; then
		# On Windows (Git Bash/MSYS), use cmd.exe to run the .bat, then the command.
		# Convert path to Windows format if cygpath exists; fall back to POSIX path otherwise.
		local BUILD_DIR_WIN="$BUILD_DIR"
		if command -v cygpath >/dev/null 2>&1; then
			BUILD_DIR_WIN="$(cygpath -w "$BUILD_DIR")"
		fi
		# Join the provided command as a single string for cmd.exe
		local CMD_STR
		CMD_STR=$(printf ' %q' "$@")
		CMD_STR=${CMD_STR:1}
		cmd.exe /C "\"${BUILD_DIR_WIN}\\conanbuild.bat\" && ${CMD_STR}"
	else
		echo "[build] WARNING: Conan build env script not found. Continuing without sourcing it."
		"$@"
	fi
}

# ----------------------------------------
# Configure, Build, Install
# ----------------------------------------
echo "[build] Configuring with CMake..."
run_in_conan_env cmake -S "$ROOT_DIR/standalone" -B "$BUILD_DIR" \
	-DCMAKE_TOOLCHAIN_FILE="conan_toolchain.cmake" \
	-DCMAKE_BUILD_TYPE=Debug \
	-DCMAKE_INSTALL_PREFIX="$INSTALL_DIR"

echo "[build] Building..."
run_in_conan_env cmake --build "$BUILD_DIR" -j "${JOBS:-16}"

echo "[build] Installing..."
run_in_conan_env cmake --build "$BUILD_DIR" --target install -j "${JOBS:-16}"

echo "[build] Done. Artifacts in: $BUILD_DIR and $INSTALL_DIR"
