#!/bin/bash

set -e

if [ "${TARGET}" = "macos-native" ] || [ "${TARGET}" = "macos-universal" ]; then
    exit 0
fi

if [ "${TARGET}" = "macos" ]; then
    # sudo rm /etc/apt/sources.list.d/cassandra.list
    # sudo rm /etc/apt/sources.list.d/chris-lea-redis-server.list
    # sudo rm /etc/apt/sources.list.d/computology_apt-backport.list
    # sudo rm /etc/apt/sources.list.d/couchdb.list
    # sudo rm /etc/apt/sources.list.d/docker.list
    # sudo rm /etc/apt/sources.list.d/github_git-lfs.list
    # sudo rm /etc/apt/sources.list.d/git-ppa.list
    sudo rm /etc/apt/sources.list.d/google-chrome.list
    # sudo rm /etc/apt/sources.list.d/heroku-toolbelt.list
    # sudo rm /etc/apt/sources.list.d/mongodb-3.4.list
    # sudo rm /etc/apt/sources.list.d/openjdk-r-java-ppa.list
    # sudo rm /etc/apt/sources.list.d/pgdg.list
    # sudo rm /etc/apt/sources.list.d/pollinate.list
    # sudo rm /etc/apt/sources.list.d/rabbitmq_rabbitmq-server.list
    # sudo rm /etc/apt/sources.list.d/webupd8team-java-ppa.list
    sudo add-apt-repository -y ppa:kxstudio-debian/kxstudio
    sudo add-apt-repository -y ppa:kxstudio-debian/mingw
    sudo add-apt-repository -y ppa:kxstudio-debian/toolchain
    sudo apt-get update -qq
    sudo apt-get install kxstudio-repos
    sudo apt-get update -qq
    exit 0
fi

if [ "${TARGET}" = "linux" ] || [ "${TARGET}" = "win32" ] || [ "${TARGET}" = "win64" ]; then
    wget -qO- https://dl.winehq.org/wine-builds/winehq.key | sudo apt-key add -
    sudo apt-add-repository -y 'deb https://dl.winehq.org/wine-builds/ubuntu/ focal main'
    sudo dpkg --add-architecture i386
    sudo apt-get update -qq
    sudo apt-get install -y -o APT::Immediate-Configure=false libc6 libc6:i386 libgcc-s1:i386
    sudo apt-get install -y -f
    exit 0
fi
