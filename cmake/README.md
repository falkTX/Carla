# Carla cmake build setup

This directory contains a cmake build setup for a few Carla libraries and tools.  
It is not meant as a stock replacement for the regular Carla build, but simply as a way to more easily integrate Carla into other projects that already use cmake.  
The cmake setup also allows building Carla with MSVC, which is not possible with the regular Makefile system.

Note that it is really only a small subset of Carla that ends up being built this way.  
Changing the main Carla build system to cmake is undesired and unwanted.
