#!/usr/bin/env bash
# Installs the system packages needed to build blackhole_simulator
# on Debian/Ubuntu (including WSL).
set -euo pipefail

if [[ $EUID -ne 0 ]]; then
    SUDO=sudo
else
    SUDO=
fi

PACKAGES=(
    build-essential   # gcc, g++, make
    libgl1-mesa-dev   # OpenGL
    libglu1-mesa-dev  # GLU
    mesa-utils        # glxinfo, glxgears for checking the OpenGL setup
    libglfw3-dev      # GLFW: windows and input
    libglew-dev       # GLEW: OpenGL function loader
    libglm-dev        # GLM: vector and matrix math
    fonts-dejavu-core # DejaVu Sans: UI font with Cyrillic and arrows
)

$SUDO apt-get update
$SUDO apt-get install -y "${PACKAGES[@]}"
