#!/usr/bin/make -f
# Makefile for carla tests #
# ------------------------ #
# Created by falkTX
#

# --------------------------------------------------------------

CXX ?= g++

BUILD_FLAGS = -Wall -Wextra -Werror -pipe -DDEBUG -O0 -g

# --------------------------------------------------------------

all: test

test: jack-enum-test-func.cpp jack-enum-test-main.cpp
	$(CXX) $^ $(BUILD_FLAGS) -o $@

# --------------------------------------------------------------

clean:
	rm -f test

# --------------------------------------------------------------
