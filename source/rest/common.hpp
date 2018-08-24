/*
 * Carla REST API Server
 * Copyright (C) 2018 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * For a full copy of the GNU General Public License see the doc/GPL.txt file.
 */

#ifndef REST_COMMON_HPP_INCLUDED
#define REST_COMMON_HPP_INCLUDED

#include "buffers.hpp"

#include "CarlaBackend.h"
#include "CarlaUtils.hpp"

#include <restbed>

using restbed::Request;
using restbed::Resource;
using restbed::Service;
using restbed::Session;
using restbed::Settings;
using restbed::BAD_REQUEST;
using restbed::OK;

CARLA_BACKEND_USE_NAMESPACE;

#endif // REST_COMMON_HPP_INCLUDED
