#!/bin/bash

# Must be run from project's root directory

mkdir -p build/test
clang++ test/src/test.cpp -c -o build/test/test.o -Ibuild/_include -Ibuild/_include/test -D PLATFORM_POSIX -g -Wall -Wextra

mkdir -p build
clang++          \
  build/test/*.o \
  -o build/test.out
