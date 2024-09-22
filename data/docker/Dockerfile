FROM ubuntu:20.04
LABEL maintainer="falkTX <falktx@falktx.com>"
ENV DEBIAN_FRONTEND noninteractive

# enable i386
RUN dpkg --add-architecture i386

# update system
RUN apt-get update -qq && apt-get upgrade -qqy && apt-get clean

# install packages needed for build
RUN apt-get install -qqy --no-install-recommends ca-certificates g++-multilib git libgl-dev make openssl pkg-config wget && \
    apt-get clean

# install newer wine
RUN mkdir -pm755 /etc/apt/keyrings && \
    wget -O /etc/apt/keyrings/winehq-archive.key https://dl.winehq.org/wine-builds/winehq.key && \
    wget -NP /etc/apt/sources.list.d/ https://dl.winehq.org/wine-builds/ubuntu/dists/focal/winehq-focal.sources

RUN apt-get update -qq && \
    apt-get install -qqy --no-install-recommends winehq-stable wine-stable-dev && \
    apt-get clean

# fetch Carla
RUN git clone --recursive https://github.com/falkTX/Carla.git --depth=1 # v4
WORKDIR /Carla

# build plugin-wine 32bit
RUN apt-get install -yqq --no-install-recommends libx11-dev:i386 && apt-get clean
RUN CFLAGS=-m32 CXXFLAGS=-m32 LDFLAGS=-m32 make plugin plugin-wine
RUN mkdir bin32
RUN mv bin/CarlaVst*ShellBridged.dll* bin32
RUN make distclean

# build plugin-wine 64bit
RUN apt-get install -yqq --no-install-recommends libx11-dev:amd64 && apt-get clean
RUN CFLAGS=-m64 CXXFLAGS=-m64 LDFLAGS=-m64 make plugin plugin-wine
RUN mkdir bin64
RUN mv bin/CarlaVst*ShellBridged.dll* bin64
RUN make distclean

# build Carla wine bridges
RUN make wine32 wine64

# CMD for inspection
CMD ["bash"]
