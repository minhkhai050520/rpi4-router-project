FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

# Install required Buildroot dependency packages
RUN apt-get update && apt-get install -y \
    build-essential \
    bash \
    bc \
    binutils \
    bzip2 \
    cpio \
    diffutils \
    file \
    findutils \
    g++ \
    gcc \
    git \
    gzip \
    libncurses5-dev \
    libssl-dev \
    make \
    patch \
    perl \
    pkg-config \
    python3 \
    rsync \
    sed \
    tar \
    unzip \
    wget \
    locales \
    sudo \
    && rm -rf /var/lib/apt/lists/*

# Configure UTF-8 locale (required by Buildroot)
RUN locale-gen en_US.UTF-8
ENV LANG=en_US.UTF-8 \
    LANGUAGE=en_US:en \
    LC_ALL=en_US.UTF-8

# Add ARGs to receive UID/GID from the host
ARG USER_ID=1000
ARG GROUP_ID=1000

# Create builder user with matching UID/GID to the host user
RUN groupadd -g ${GROUP_ID} rpi4_builder && \
    useradd -m -u ${USER_ID} -g ${GROUP_ID} -s /bin/bash rpi4_builder && \
    echo "rpi4_builder ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers

USER rpi4_builder
WORKDIR /workspace