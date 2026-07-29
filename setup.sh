#!/usr/bin/env bash
set -euo pipefail

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILDROOT_TAG="${BUILDROOT_TAG:-2026.05.1}"
BUILDROOT_DIR="${BUILDROOT_DIR:-$PROJECT_DIR/buildroot}"
SOURCE_CONFIG_DIR="$PROJECT_DIR/khainpm"
TARGET_CONFIG_DIR="$BUILDROOT_DIR/board/khainpm"

log() {
    echo "[setup] $*"
}

require_command() {
    local cmd="$1"
    if ! command -v "$cmd" >/dev/null 2>&1; then
        log "Missing required command: $cmd"
        log "Please install it and run the script again."
        exit 1
    fi
}

check_prerequisites() {
    log "Checking prerequisites..."
    local required=(git docker)
    for cmd in "${required[@]}"; do
        require_command "$cmd"
    done
    log "Prerequisites OK."
}

ensure_buildroot() {
    if [ -d "$BUILDROOT_DIR/.git" ]; then
        log "Buildroot already exists at $BUILDROOT_DIR"
        return 0
    fi

    if [ -e "$BUILDROOT_DIR" ]; then
        log "Path exists but is not a valid Buildroot repository: $BUILDROOT_DIR"
        exit 1
    fi

    log "Cloning Buildroot ${BUILDROOT_TAG} (shallow clone)..."
    git clone --depth 1 --branch "$BUILDROOT_TAG" https://github.com/buildroot/buildroot.git "$BUILDROOT_DIR"
}

link_config() {
    if [ ! -d "$SOURCE_CONFIG_DIR" ]; then
        log "Source directory not found: $SOURCE_CONFIG_DIR"
        exit 1
    fi

    if [ ! -d "$BUILDROOT_DIR/board" ]; then
        log "Buildroot board directory does not exist: $BUILDROOT_DIR/board"
        exit 1
    fi

    log "Linking khainpm into Buildroot board directory"
    cd "$BUILDROOT_DIR/board"
    ln -sfn ../../khainpm khainpm
}

main() {
    check_prerequisites
    ensure_buildroot
    link_config
    log "Setup completed successfully."
}

main "$@"
