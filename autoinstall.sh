#! /bin/sh

# Simple script to install PAGAI's dependencies, and then to compile
# it.

cd external/ && make
cmake .
make
