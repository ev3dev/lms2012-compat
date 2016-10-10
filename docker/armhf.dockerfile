FROM ev3dev/debian-jessie-cross
RUN sudo apt-get update && \
    DEBIAN_FRONTEND=noninteractive sudo apt-get install --yes --no-install-recommends \
        cmake \
        imagemagick \
        libasound2-dev:armhf \
        libdbus-1-dev:armhf \
        libglib2.0-dev:armhf \
        libgrx-3.0-dev:armhf \
        libudev-dev:armhf \
        libusb-1.0-0-dev:armhf \
        lmsasm \
        pandoc \
        pkg-config \
        sox
ENV PKG_CONFIG_PATH=/usr/lib/arm-linux-gnueabihf/pkgconfig
