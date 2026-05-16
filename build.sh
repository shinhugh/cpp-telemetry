#!/bin/bash

# Must be run from project's root directory

mkdir -p build/_include/telemetry
cp telemetry/include/* build/_include/telemetry/

mkdir -p build/telemetry
clang++ telemetry/src/living_span.cpp -c -o build/telemetry/living_span.o -Ibuild/_include -Ibuild/_include/telemetry -D PLATFORM_POSIX -g -Wall -Wextra
clang++ telemetry/src/log.cpp         -c -o build/telemetry/log.o         -Ibuild/_include -Ibuild/_include/telemetry -D PLATFORM_POSIX -g -Wall -Wextra
clang++ telemetry/src/log_value.cpp   -c -o build/telemetry/log_value.o   -Ibuild/_include -Ibuild/_include/telemetry -D PLATFORM_POSIX -g -Wall -Wextra
clang++ telemetry/src/trace.cpp       -c -o build/telemetry/trace.o       -Ibuild/_include -Ibuild/_include/telemetry -D PLATFORM_POSIX -g -Wall -Wextra
llvm-ar rcs build/libtelemetry.a build/telemetry/*.o

mkdir -p build/test
clang++ test/src/test.cpp -c -o build/test/test.o -Ibuild/_include -Ibuild/_include/test -D PLATFORM_POSIX -g -Wall -Wextra

mkdir -p build
clang++          \
  build/test/*.o \
  -Lbuild        \
  -ltelemetry    \
  -o build/test.out
