#!/bin/bash
# NIKOS Install Script
# Automatically detects OS, installs dependencies, and downloads the latest release from GitHub.

set -e

REPO="alternative-intelligence-cp/nikos"
RELEASE_TAG="v2.0.0"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}==============================================${NC}"
echo -e "${BLUE}     NIKOS - Automatic Installer Script       ${NC}"
echo -e "${BLUE}==============================================${NC}"

# Check for root
if [ "$EUID" -ne 0 ]; then
  echo -e "${RED}Please run this script as root (sudo).${NC}"
  exit 1
fi

# Detect OS
if [ -f /etc/os-release ]; then
    . /etc/os-release
    OS=$ID
    VERSION=$VERSION_ID
else
    echo -e "${RED}Unsupported OS. Cannot detect Linux distribution.${NC}"
    exit 1
fi

echo -e "Detected OS: ${GREEN}${OS} ${VERSION}${NC}"

install_debian() {
    echo -e "Installing dependencies for Debian/Ubuntu..."
    apt-get update
    apt-get install -y wget curl cmake libboost-all-dev libgmp-dev \
        libsqlite3-dev libtbb-dev libedit-dev zlib1g-dev python3 \
        llvm-20-dev clang-20 libclang-20-dev

    URL="https://github.com/${REPO}/releases/download/${RELEASE_TAG}/nikos-${RELEASE_TAG}-Linux.deb"
    
    echo -e "Downloading NIKOS ${RELEASE_TAG} (.deb)..."
    wget -qO /tmp/nikos.deb "$URL"
    
    echo -e "Installing NIKOS package..."
    dpkg -i /tmp/nikos.deb || apt-get install -f -y
    
    rm -f /tmp/nikos.deb
}

install_rhel() {
    echo -e "Installing dependencies for RHEL/Fedora..."
    if command -v dnf >/dev/null 2>&1; then
        PM="dnf"
    else
        PM="yum"
    fi
    
    $PM install -y wget curl cmake boost-devel gmp-devel \
        sqlite-devel tbb-devel libedit-devel zlib-devel python3 \
        llvm20-devel clang20
        
    URL="https://github.com/${REPO}/releases/download/${RELEASE_TAG}/nikos-${RELEASE_TAG}-Linux.rpm"
    
    echo -e "Downloading NIKOS ${RELEASE_TAG} (.rpm)..."
    wget -qO /tmp/nikos.rpm "$URL"
    
    echo -e "Installing NIKOS package..."
    $PM install -y /tmp/nikos.rpm
    
    rm -f /tmp/nikos.rpm
}

if [[ "$OS" == "ubuntu" || "$OS" == "debian" || "$OS" == "linuxmint" ]]; then
    install_debian
elif [[ "$OS" == "fedora" || "$OS" == "rhel" || "$OS" == "centos" ]]; then
    install_rhel
else
    echo -e "${RED}Unsupported OS: ${OS}. Please build from source.${NC}"
    exit 1
fi

echo -e "${GREEN}NIKOS installation complete!${NC}"
echo "You can now run 'ikos-analyzer --version' to verify."
