#!/bin/bash

valgrind --tool=memcheck --leak-check=full --show-leak-kinds=all --suppressions=./data/valgrind.supp -- ./bin/carla-bridge-native internal "" carlapatchbay &
PID=$!

while true; do
    if jack_lsp | grep -q Carla-Patchbay:output_1; then
        jack_connect Carla-Patchbay:output_2 system:playback_2
        jack_connect Carla-Patchbay:output_1 system:playback_1
        break
    else
        sleep 1
    fi
done

wait ${PID}
