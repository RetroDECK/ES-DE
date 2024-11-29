#!/bin/bash

# WARNING: run this script from the project root folder, not from here!!

cmake -DRETRODECK=on -DCMAKE_INSTALL_PREFIX=/app . && make && make install