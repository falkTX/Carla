#pragma once
#include <cstddef>

namespace rtosc{struct Ports; struct RtData;}

size_t subtree_serialize(char *buffer, size_t buffer_size,
        void *object, rtosc::Ports *ports);

void subtree_deserialize(char *buffer, size_t buffer_size,
        void *object, rtosc::Ports *ports, rtosc::RtData &d);
