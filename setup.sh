#!/bin/bash

# Setup script for Conway Lockscreen
# This script will:
# 1. Install hyprlock if not already installed
# 2. Set up configuration files
# 3. Install and compile the Conway lockscreen script

if [ "$USER" = "root" ]; then
    USER_HOME="/root"
else
    USER_HOME=$(eval echo ~$USER)
fi

CONFIG_DIR="$USER_HOME/.config/hypr"
SCRIPTS_DIR="$CONFIG_DIR/conway_lockscreen"

command_exists() {
    command -v "$1" >/dev/null 2>&1
}

run_with_sudo() {
    if [ "$EUID" -eq 0 ]; then
        "$@"
    else
        sudo "$@"
    fi
}

echo "Checking for hyprlock installation..."
if ! command_exists hyprlock; then
    echo "hyprlock not found. Installing hyprlock..."

    if command_exists apt-get; then
        echo "Attempting to install hyprlock via apt-get..."
        run_with_sudo apt-get update
        run_with_sudo apt-get install -y hyprlock
    elif command_exists pacman; then
        echo "Attempting to install hyprlock via pacman..."
        run_with_sudo pacman -S --noconfirm hyprlock
    elif command_exists dnf; then
        echo "Attempting to install hyprlock via dnf..."
        run_with_sudo dnf install -y hyprlock
    else
        echo "Package manager not found or hyprlock not available. Building from source..."
        echo "Cloning hyprlock repository..."

        TEMP_DIR=$(mktemp -d)
        cd "$TEMP_DIR" || exit 1

        git clone https://github.com/hyprwm/hyprlock.git
        cd hyprlock || exit 1

        echo "Building hyprlock..."
        make
        run_with_sudo make install

        cd ..
        rm -rf "$TEMP_DIR"
    fi

    if command_exists hyprlock; then
        echo "hyprlock installed successfully."
    else
        echo "Failed to install hyprlock. Please install it manually."
        exit 1
    fi
else
    echo "hyprlock is already installed."
fi

echo "Setting up configuration directories..."
mkdir -p "$CONFIG_DIR"
mkdir -p "$SCRIPTS_DIR"

echo "Copying configuration files..."
rm -f "$CONFIG_DIR/hyprlock.conf"
rm -f "$SCRIPTS_DIR/config.json"
rm -f "$SCRIPTS_DIR/conway.cpp"

cp hyprlock.conf "$CONFIG_DIR/hyprlock.conf"
cp config.json "$SCRIPTS_DIR/config.json"
cp conway.cpp "$SCRIPTS_DIR/conway.cpp"

echo "Compiling conway.cpp..."
cd "$SCRIPTS_DIR" || exit 1
g++ -std=c++17 -o conway conway.cpp

if [ $? -eq 0 ]; then
    echo "conway.cpp compiled successfully to conway"
else
    echo "Failed to compile conway.cpp"
    exit 1
fi

chmod +x "$SCRIPTS_DIR/conway"

echo "Setup complete!"
echo ""
echo "Configuration can be modified at: $SCRIPTS_DIR/config.json"
echo ""
echo "To use this lockscreen, add the following to your Hyprland config:"
echo "  exec-then = hyprlock "
echo ""
echo "Or manually test with:"
echo "  hyprlock "
echo ""
echo "Note: If you ran this script with sudo, the files are placed in root's home directory."
echo "  You may want to move them to your user's home directory or run the script without sudo."