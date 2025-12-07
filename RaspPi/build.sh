#!/bin/bash

echo "Building Smart Plug Server..."

mkdir -p build
cd build

cmake -DCMAKE_BUILD_TYPE=Release ..

make -j4

echo "Build completed!"
echo "Binary: build/smart_plug_server"