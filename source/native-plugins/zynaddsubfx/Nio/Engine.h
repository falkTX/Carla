/*
  ZynAddSubFX - a software synthesizer

  Engine.h - Audio Driver base class
  Copyright (C) 2009-2010 Mark McCurry
  Author: Mark McCurry

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#ifndef ENGINE_H
#define ENGINE_H
#include <string>
/**Marker for input/output driver*/
class Engine
{
    public:
        Engine();
        virtual ~Engine();

        /**Start the Driver with all capabilities
         * @return true on success*/
        virtual bool Start() = 0;
        /**Completely stop the Driver*/
        virtual void Stop() = 0;

        std::string name;
};
#endif
