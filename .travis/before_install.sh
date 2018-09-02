#!/bin/bash

set -e

sudo add-apt-repository ppa:kxstudio-debian/kxstudio -y
sudo add-apt-repository ppa:kxstudio-debian/mingw -y
sudo add-apt-repository ppa:kxstudio-debian/toolchain -y
sudo apt-get update -qq
sudo apt-get install kxstudio-repos
sudo apt-get update -qq
