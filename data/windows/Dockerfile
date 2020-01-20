FROM ubuntu:18.04
MAINTAINER falkTX <falktx@falktx.com>
ENV DEBIAN_FRONTEND noninteractive

# enable i386
RUN dpkg --add-architecture i386

RUN echo "deb [arch=amd64,i386] http://archive.ubuntu.com/ubuntu/ bionic main restricted universe multiverse"            > /etc/apt/sources.list && \
    echo "deb [arch=amd64,i386] http://archive.ubuntu.com/ubuntu/ bionic-updates main restricted universe multiverse"   >> /etc/apt/sources.list && \
    echo "deb [arch=amd64,i386] http://archive.ubuntu.com/ubuntu/ bionic-backports main restricted universe multiverse" >> /etc/apt/sources.list && \
    echo "deb [arch=amd64,i386] http://security.ubuntu.com/ubuntu bionic-security main restricted universe multiverse"  >> /etc/apt/sources.list

# update and upgrade system
RUN apt-get update && apt-get upgrade -qy && apt-get clean

# install packages
RUN apt-get install -qy mingw-w64 && \
    apt-get install -qy qemu-user-static libwine-development-dev wine64-development-tools && \
    apt-get install -qy locales acl bash bash-completion git nano patch python3 tar wget && \
    apt-get install -qy automake binutils build-essential cmake libglib2.0-dev-bin libtool-bin && \
    apt-get clean

# basic setup
RUN locale-gen en_US.UTF-8
RUN echo "source /etc/bash_completion" >> $HOME/.bashrc

# user configurations
ENV USER builder
ENV HOME /home/$USER

# create user
RUN useradd -d $HOME -m -G sudo $USER

# switch user
USER $USER

# checkout scripts dir
RUN mkdir $HOME/scripts
RUN mkdir $HOME/scripts/patches
COPY common.env build-*.sh $HOME/scripts/
COPY patches/* $HOME/scripts/patches/

# build deps
WORKDIR $HOME/scripts
RUN ./build-deps.sh
RUN ./build-pyqt.sh

# CMD
CMD ["bash"]
