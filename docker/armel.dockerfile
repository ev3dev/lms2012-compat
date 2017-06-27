FROM ev3dev/debian-stretch-cross
RUN sudo apt-get update && \
    DEBIAN_FRONTEND=noninteractive sudo apt-get install --yes --no-install-recommends \
        cmake \
        imagemagick \
        libasound2-dev:armel \
        libdbus-1-dev:armel \
        libglib2.0-dev:armel \
        libgrx-3.0-dev:armel \
        libudev-dev:armel \
        libusb-1.0-0-dev:armel \
        lmsasm \
        pandoc \
        pkg-config \
        sox
ENV PKG_CONFIG_PATH=/usr/lib/arm-linux-gnueabi/pkgconfig
