FROM ubuntu:latest

LABEL maintainer="ConnorK"
RUN echo 'debconf debconf/frontend select Noninteractive' | debconf-set-selections

# Update/upgrade
RUN apt-get update && apt-get upgrade -y

# Install dependencies.
RUN apt-get install -y build-essential mercurial make cmake autoconf automake \
    && apt-get install -y libtool libasound2-dev libpulse-dev libaudio-dev libx11-dev libxext-dev \
    && apt-get install -y libxrandr-dev libxcursor-dev libxi-dev libxinerama-dev libxxf86vm-dev \
    && apt-get install -y libxss-dev libgl1-mesa-dev libdbus-1-dev libudev-dev libgles2-mesa-dev \
    && apt-get install -y libegl1-mesa-dev libibus-1.0-dev fcitx-libs-dev libsamplerate0-dev \
    && apt-get install -y libsndio-dev libwayland-dev libxkbcommon-dev

# Install libsdl2
RUN apt-get install -y libsdl2-dev

# Setup build-path
RUN mkdir /build-path \
    && cd /build-path

WORKDIR /build-path

# Entrypoint for build
ENTRYPOINT ["make"]
